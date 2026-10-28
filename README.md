# Smart Battery

Machine-learning-based estimation for a battery pack, designed to run **on a microcontroller**. Models are trained in Python, exported to TensorFlow Lite / C headers, and validated on real hardware with an Arduino test bench.

## Overview

The goal of this project is to run a lightweight battery-state estimator directly on embedded hardware, without a PC or cloud connection. Two model families are explored and compared:

| Model | Format | Where |
|---|---|---|
| Small neural network (MLP, 4 → 16 → 8 → 1) | TFLite → plain C arrays | `tflite_models/`, `model_params.h` |
| Random forest | C header (inference code + trees) | `random_forest.h` |

The neural network takes **4 input features**, standardised with a per-feature mean/standard deviation (`mu`, `sigma`), and produces **a single output**.  

Full design notes and the control concept are in [`Smart Battery Control.pdf`](Smart%20Battery%20Control.pdf).

## Repository structure

```
Smart_battery/
├── Arduino Testing/              # Arduino sketches for on-target testing
├── Hardware/                     # Hardware design files
├── Model Validation - Testing/   # Scripts and data to validate the models
├── tflite_models/                # Exported TensorFlow Lite models
├── training_explained (1).ipynb  # Training pipeline, step by step
├── conversion.ipynb              # TFLite model -> C header conversion
├── model_params.h                # Auto-generated NN weights, biases, scaler
├── random_forest.h               # Random forest exported for C/C++ (~3.6 MB)
└── Smart Battery Control.pdf     # Project documentation
```

## How it works

1. **Train** – `training_explained (1).ipynb` walks through data preparation, feature scaling, and training of the models.
2. **Export** – the trained network is saved as a `.tflite` model in `tflite_models/`.
3. **Convert** – `conversion.ipynb` extracts the weights from the TFLite file and writes `model_params.h`, which contains:
   - `mu[]`, `sigma[]` – input standardisation parameters
   - `W1`, `b1`, `W2`, `b2`, `W3`, `b3` – layer weights and biases
   - `N_IN`, `N_H1`, `N_H2` – layer sizes
4. **Deploy** – the header is included in an Arduino sketch (see `Arduino Testing/`), where inference is a few matrix-vector products in plain C, with no TFLite runtime required.
5. **Validate** – `Model Validation - Testing/` compares on-device predictions against the Python reference.

## Getting started

### Requirements

- Python 3.x with Jupyter
- Arduino IDE or PlatformIO
- Arduino R4 WiFi

### Reproduce the models

```bash
git clone https://github.com/tommasolanzini/Smart_battery.git
cd Smart_battery
pip install jupyter numpy pandas scikit-learn tensorflow   # adjust to your environment
jupyter notebook "training_explained (1).ipynb"
```

### Run on the microcontroller

1. Copy `model_params.h` (and/or `random_forest.h`) next to your sketch.
2. Open a sketch from `Arduino Testing/` in the Arduino IDE.
3. Select your board and port, then upload.

## Author

- **Giacomo Bellumè**
- **Riccardo Granata**
- **Daniele Fagotti**
- **Tommaso Lanzini** 
