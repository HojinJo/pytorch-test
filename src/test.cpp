#include <torch/script.h>
#include <torch/torch.h>
#include <typeinfo>
#include <iostream>
#include <inttypes.h>
#include <functional>
#include <memory>
#include <stdlib.h>
#include <pthread.h>

#include "test.h"
#include "alex.h"
#include "vgg.h"
#include "resnet18.h"
#include "densenet.h"

#define n_dense 0
#define n_res 0
#define n_alex 0
#define n_vgg 1
#define n_wide 0


#define n_threads 3

extern void *predict_alexnet(Net *input);
extern void *predict_vgg(Net *input);
extern void *predict_resnet18(Net *input);
extern void *predict_densenet(Net *input);

namespace F = torch::nn::functional;
using namespace std;

void print_script_module(const torch::jit::script::Module& module, size_t spaces) {
    for (const auto& sub_module : module.named_children()) {
        if (!sub_module.name.empty()) {
            std::cout << std::string(spaces, ' ') << sub_module.value.type()->name().value().name()
                << " " << sub_module.name << "\n";    
        }
        print_script_module(sub_module.value, spaces + 2);
    }
}

void print_vector(vector<int> v){
	for(int i=0;i<v.size();i++){
		cout<<v[i]<<" ";
	}
	cout<<"\n";
}

threadpool thpool;
pthread_cond_t* cond_t;
pthread_mutex_t* mutex_t;
int* cond_i;

int main(int argc, const char* argv[]) {

  int n_all = n_alex + n_vgg + n_res + n_dense + n_wide;

  thpool = thpool_init(n_threads);

  torch::jit::script::Module denseModule[n_dense];
  torch::jit::script::Module resModule[n_res];
  torch::jit::script::Module alexModule[n_alex];
  torch::jit::script::Module vggModule[n_vgg];
  torch::jit::script::Module wideModule[n_wide];
  
  try {
    for (int i=0;i<n_dense;i++){    
    	denseModule[i] = torch::jit::load("../densenet_model.pt");
    }
    for (int i=0;i<n_res;i++){    
    	resModule[i] = torch::jit::load("../resnet_model.pt");
    }
    for(int i=0;i<n_alex;i++){
    	alexModule[i] = torch::jit::load("../alexnet_model.pt");
    }
    for (int i=0;i<n_vgg;i++){    
    	vggModule[i] = torch::jit::load("../vgg_model.pt");
    }
    for (int i=0;i<n_wide;i++){    
    	wideModule[i] = torch::jit::load("../wideresnet_model.pt");
    }
  }
  catch (const c10::Error& e) {
    cerr << "error loading the model\n";
    return -1;
  }
  cout<<"*** Model Load compelete ***"<<"\n";

  cond_t = (pthread_cond_t *)malloc(sizeof(pthread_cond_t) * n_all);
  mutex_t = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t) * n_all);
  cond_i = (int *)malloc(sizeof(int) * n_all);

  for (int i = 0; i < n_all; i++)
  {
      pthread_cond_init(&cond_t[i], NULL);
      pthread_mutex_init(&mutex_t[i], NULL);
      cond_i[i] = 0;
  }


  vector<torch::jit::IValue> inputs;
  //module.to(at::kCPU);
   
  torch::Tensor x = torch::ones({1, 3, 224, 224});
  inputs.push_back(x);
  
  Net net_input_dense[n_dense];
  Net net_input_res[n_res];
  Net net_input_alex[n_alex];
  Net net_input_vgg[n_vgg];
  Net net_input_wide[n_wide];

  pthread_t networkArray_dense[n_dense];
  pthread_t networkArray_res[n_res];
  pthread_t networkArray_alex[n_alex];
  pthread_t networkArray_vgg[n_vgg];
  pthread_t networkArray_wide[n_wide];
  
  std::vector<torch::jit::Module> densechild[n_dense];
  std::vector<torch::jit::Module> reschild[n_res];
  std::vector<torch::jit::Module> alexchild[n_alex];
  std::vector<torch::jit::Module> vggchild[n_vgg];
  std::vector<torch::jit::Module> widechild[n_wide];  
  
  std::vector<pair<int,int>> denseblock;
  std::vector<pair<int,int>> resblock;
  std::vector<pair<int,int>> wideblock;

  for(int i=0;i<n_dense;i++){
	  get_submodule_densenet(denseModule[i], densechild[i],denseblock);
    std::cout << "End get submodule_densenet "<< i << "\n";
    cout<<densechild[i].size()<<"\n";
	  net_input_dense[i].child = densechild[i];
    std::cout<<"11111"<<"\n";
    net_input_dense[i].block = denseblock;
    std::cout<<"wwwww"<<"\n";
    net_input_dense[i].layer = (Layer*)malloc(sizeof(Layer)*densechild[i].size());
	  net_input_dense[i].input = inputs;
    cout<<"qqqqqqqqqqqqqqqqqqqq\n";
    net_input_dense[i].index_n = i;
    cout<<"qqtttttttttttttttttttttttttt\n";
  }

  for(int i=0;i<n_res;i++){
	  get_submodule_resnet18(resModule[i], reschild[i],resblock);
    std::cout << "End get submodule_resnet "<< i << "\n";
	  net_input_res[i].child = reschild[i];
    net_input_res[i].block = resblock;
	  net_input_res[i].layer = (Layer*)malloc(sizeof(Layer)*reschild[i].size());
	  net_input_res[i].input = inputs;
    net_input_res[i].index_n = i+n_dense;
  }

  for(int i=0;i<n_alex;i++){
	  get_submodule_alexnet(alexModule[i], alexchild[i]);
    //get_submodule_alexnet(alexModule[i], alexchild[i]);
    //std::cout<<alexchild[i].size()<<'\n';
    std::cout << "End get submodule_alex " << i <<"\n";
	  //net_input_alex[i] = (Net *)malloc(sizeof(Net));
    //std::cout<<"11111"<<"\n";
    net_input_alex[i].child = alexchild[i];
    //std::cout<<"22222"<<"\n";
	  net_input_alex[i].layer = (Layer*)malloc(sizeof(Layer)*alexchild[i].size());
	  net_input_alex[i].input = inputs;
    //std::cout<<"33333"<<"\n";
    net_input_alex[i].index_n = i+ n_res + n_dense;
    //std::cout<<"4444"<<"\n";
  }

  for(int i=0;i<n_vgg;i++){
	  get_submodule_vgg(vggModule[i], vggchild[i]);
    std::cout << "End get submodule_vgg " << i << "\n";
	  //net_input_vgg[i] = (Net *)malloc(sizeof(Net));
	  net_input_vgg[i].child = vggchild[i];
	  net_input_vgg[i].layer = (Layer*)malloc(sizeof(Layer)*vggchild[i].size());
	  net_input_vgg[i].input = inputs;
    net_input_vgg[i].index_n = i + n_alex + n_res + n_dense;
  }

  for(int i=0;i<n_wide;i++){
	  get_submodule_resnet18(wideModule[i], widechild[i],wideblock);
    std::cout << "End get submodule_widenet "<< i << "\n";
	  net_input_wide[i].child = widechild[i];
    net_input_wide[i].block = wideblock;
	  net_input_wide[i].layer = (Layer*)malloc(sizeof(Layer)*widechild[i].size());
	  net_input_wide[i].input = inputs;
    net_input_wide[i].index_n = i+n_alex + n_res + n_dense + n_vgg;
  }

for(int i=0;i<n_dense;i++){
  cout<<"dfdfewewfwe\n";
    if (pthread_create(&networkArray_dense[i], NULL, (void *(*)(void*))predict_densenet, &net_input_dense[i]) < 0){
      perror("thread error");
      exit(0);
    }
  }
  for(int i=0;i<n_res;i++){
    if (pthread_create(&networkArray_res[i], NULL, (void *(*)(void*))predict_resnet18, &net_input_res[i]) < 0){
      perror("thread error");
      exit(0);
    }
  }
  for(int i=0;i<n_alex;i++){
    if (pthread_create(&networkArray_alex[i], NULL, (void *(*)(void*))predict_alexnet, &net_input_alex[i]) < 0){
      perror("thread error");
      exit(0);
    }
  }
  for(int i=0;i<n_vgg;i++){
	  if (pthread_create(&networkArray_vgg[i], NULL, (void *(*)(void*))predict_vgg, &net_input_vgg[i]) < 0){
      perror("thread error");
      exit(0);
    }
  }
  for(int i=0;i<n_wide;i++){
    if (pthread_create(&networkArray_wide[i], NULL, (void *(*)(void*))predict_resnet18, &net_input_wide[i]) < 0){
      perror("thread error");
      exit(0);
    }
  }

  for (int i = 0; i < n_dense; i++){
    pthread_join(networkArray_dense[i], NULL);
  }
  for (int i = 0; i < n_res; i++){
    pthread_join(networkArray_res[i], NULL);
  }
  for (int i = 0; i < n_alex; i++){
    pthread_join(networkArray_alex[i], NULL);
  }
  for (int i = 0; i < n_vgg; i++){
    pthread_join(networkArray_vgg[i], NULL);
  }
  for (int i = 0; i < n_wide; i++){
    pthread_join(networkArray_wide[i], NULL);
  }
  free(cond_t);
  free(mutex_t);
  free(cond_i);
}
