# DroughtBot – Code changes

*(This is AI generated summary of the things I have changed)*

This note summarizes the main differences between the old and new Raspberry Pi code.

## `pi_logic.py`

### 1. Clearer process states
The new version uses `_busy` and `_pending` to distinguish between:
- **Idle:** ready to start a cycle.
- **Waiting for a node:** a request is being processed.
- **Waiting for OK:** the user must confirm before the next step.

Previously, `_pending` was used for confirmations, but there was no separate busy state.

### 2. New series-reading workflow
The new code can register QR codes for every plant in a series:
- Gets the latest series from the API.
- Gets the plants belonging to that series.
- Sends the series ID and plant count to the display.
- Scans and saves each plant's QR code.
- Keeps track of scanned codes to prevent duplicates.
- Asks the user to retry when a QR code cannot be saved.

This is handled mainly by `start_reading_series()`, `read_plant_qr()` and `register_qr()`.

### 3. More button functions
A third button action, `BUTTON_OTHER`, starts the series-reading workflow when the system is idle. The existing start and abort actions have also been updated.

### 4. Improved error handling
The new code defines error codes and displays them on the inout node. It also:
- Uses `_safe()` to catch exceptions in callbacks.
- Resets the process to an idle state after errors.
- Aborts the station if necessary, for example after a watering error.
- Ignores results that arrive from an older process cycle.

### 5. Retry handling
When a node reports that it is busy, the new code can retry the request after a delay. The number of retries is limited by `MAX_RETRIES`.

## `pi_serial_handler.py`

The serial handler has changed less than the logic file. Its main responsibility is still to handle serial communication, API calls and saving node responses.

The main addition is:

- **`on_tick` callback:** The handler calls this on every main-loop iteration. `pi_logic.py` uses it to check whether a delayed retry is due.

Error handling around response-saving functions has also been broadened to catch unexpected exceptions, rather than only a selected set of exception types.

## In short

The serial handler mostly keeps the same role. The largest changes are in `pi_logic.py`: the new version supports registering all plants in a series, has clearer process states, and handles errors, retries and late responses more safely.
