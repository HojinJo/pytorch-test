#ifndef VGG_H
#define VGG_H

#include "net.h"
#include "test.h"
#include "thpool.h"

void get_submodule_vgg(torch::jit::script::Module module, std::vector<torch::jit::Module> &child);
void *predict_vgg(Net *input);
void forward_vgg(th_arg *th);

#endif
