# Pio commands

## Compile code 

```bash
pio run
```

## Upload code
### With compiling
 
```bash
pio run -t upload
``` 

### Without compiling
```bash
pio run -t nobuild -t upload
```

### On a clean build (Can be good sometimes)

```bash
pio run -t clean -t upload
```

## Serial monitor

```bash
pio device monitor
```
