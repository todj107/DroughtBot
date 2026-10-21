# Scale + pump

## demo_scale_pump 

Simple scale and pump integration, with user inputs.

### Program description

The program lets the user choose how many ml water that should be added. Then it starts the pump until the scale shows the choosen weight. 2 grams before the choosen weight is reached the pump intensity is lowered to reach more accuret result.

### Code structure

The code uses two classes
```cpp
ScaleController
```
and
```cpp
PumpController
```
Their header files are defined in `include`.