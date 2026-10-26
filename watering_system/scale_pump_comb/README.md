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


## scale_pump_stabilizer

Same as demo_scale_pump except that a scale stabilizer is used sush that the readings are more stable and less sensitive to noise.

### Code structure

The code uses two classes
```cpp
ScaleController
```
and
```cpp
PumpController
```
Where `ScaleController` uses `ScaleStabilizer` internally. Their header files are defined in `include`.

### Comment
This code works very well and have an error of approximately 0.1g when I have tried. 