#!/home/carol/radiation-benchmarks/src/cuda/resnet_torch/torch/install/bin/th

-- 
--  Copyright (c) 2016, Manuel Araoz
--  Copyright (c) 2016, Facebook, Inc.
--  All rights reserved.
--
--  This source code is licensed under the BSD-style license found in the
--  LICENSE file in the root directory of this source tree. An additional grant
--  of patent rights can be found in the PATENTS file in the same directory.
--
--  classifies an image using a trained model
--

require 'torch'
require 'paths'
require 'cudnn'
require 'cunn'
require 'image'


local lp = require 'logs_processing'
local t = require '../datasets/transforms'
local imagenetLabel = require './imagenet'


--/////////////////////////////////////////////////////

function parse_args()
  if #arg < 2 then
    io.stderr:write('Usage for normal execution: th classify.lua [MODEL] [FILE]...\n')
    io.stderr:write('Usage for radiation execution: th classifi_radiation.lua [model] \
    [radiation mode gen/rad] [txt dataset] [gold_file] [iterations -- ignored for gen]\n')
    os.exit(1)
  end
  for _, f in ipairs(arg) do
    if not paths.filep(f) then
      io.stderr:write('file not found: ' .. f .. '\n')
      os.exit(1)
    end
  end


end

function load_model()
  print("Loading Resnet models " .. arg[1])
  -- Load the model
  local model = torch.load(arg[1]):cuda()
  local softMaxLayer = cudnn.SoftMax():cuda()

  -- add Softmax layer
  model:add(softMaxLayer)

  -- Evaluate mode
  model:evaluate()

  -- The model was trained with this input normalization
  local meanstd = {
    mean = { 0.485, 0.456, 0.406 },
    std = { 0.229, 0.224, 0.225 },
  }

  local transform = t.Compose{
    t.Scale(256),
    t.ColorNormalize(meanstd),
    t.CenterCrop(224),
  }

  print("Models and Network created with sucess")

  return model, softMaxLayer, transform, meanstd
end

function normal_execution()
  local model, softMaxLayer, transform, meanstd = load_model()
  local N = 10

  for i=2,#arg do
    print(i)
    -- load the image as a RGB float tensor with values 0..1
    local img = image.load(arg[i], 3, 'float')
    local name = arg[i]:match( "([^/]+)$" )

    -- Scale, normalize, and crop the image
    img = transform(img)

    -- View as mini-batch of size 1
    local batch = img:view(1, table.unpack(img:size():totable()))

    -- Get the output of the softmax
    local output = model:forward(batch:cuda()):squeeze()

    -- Get the top 5 class indexes and probabilities
    local probs, indexes = output:topk(N, true, true)
    print('Classes for', arg[i])
    for n=1,N do
      print(probs[n], imagenetLabel[indexes[n]])
    end
    print('')

  end

end


function generate_radiation_test(arg_size)
  local model, softMaxLayer, transform, meanstd = load_model()
  local N = 10

  for i=arg_size,#arg do

    -- load the image as a RGB float tensor with values 0..1
    -- [model] [radiation mode gen/rad] [txt dataset] [gold_file] [iterations -- ignored for gen]
    local img = image.load(arg[i], 3, 'float')
    local name = arg[i]:match( "([^/]+)$" )
      

    -- Scale, normalize, and crop the image
    img = transform(img)

    -- View as mini-batch of size 1
    local batch = img:view(1, table.unpack(img:size():totable()))

    -- Get the output of the softmax
    local output = model:forward(batch:cuda()):squeeze()

    -- Get the top 5 class indexes and probabilities
    local probs, indexes = output:topk(N, true, true)
    print('Classes for', arg[i])
    for n=1,N do
      print(probs[n], imagenetLabel[indexes[n]])
    end
    print('')

  end

end


parse_args()
normal_execution()
