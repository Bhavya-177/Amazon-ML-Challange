# Business Entity Resolution Pipeline

## Instructions to Reproduce
1. Place cleaned data in `dataset/cleaned/` and raw labels in `dataset/train/`.
2. Build binary:
   ```bash
   cd business_entity_resolution/code
   mkdir -p build && cd build
   cmake ..
   make -j4