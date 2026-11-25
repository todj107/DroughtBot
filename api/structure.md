# Data base structure

## Series

| name | type | constraints | comment |
| ---- | ---- | ---- | ------- |
| id | int | auto, **primery key** | -  |
| name | text | Not null | Name of the series |
| created_at | DATETIME | not null, default CURRENT_TIMESTAMP | When the series was created |

## Plants

| name | type | constraints | comment |
| ---- | ---- | ---- | ------- |
| id | int | auto, **primery key** | - |
| serie_id | int | foregin key, refernces series(id), ON DELETE CASCADE | refernce to series id |
| qr_code | text | UNIQUE, defualt null | Plant identifier |
| target_weight | float | not null | - |
| plant_type | text | defualt null | Plant type or simular (unnecessary) |

or

| name | type | constraints | comment |
| ---- | ---- | ---- | ------- |
| id | int | auto, **primery key** | - |
| series_id | int | foregin key, refernces series(id), ON DELETE CASCADE | refernce to series id |
| qr_code | text | UNIQUE, defualt null | Plant identifier |
| initial_water | float | not null | Water to be added the first measurment |
| dry_weight | float | deafult null | Then this is weight the first time it is placed on the scale|
| type | text | defualt null | Plant type or simular (unnecessary) |

## Measurements

| name | type | constraints | comment |
| ---- | ---- | ---- | ------- |
| id | int | auto, **primery key** | - |
| plant_id | int | foregin key, refernces plants(id), ON DELETE CASCADE | refernce to the plant id |
| measured_at | DATETIME | not null, defualt CURRENT_TIMESTAMP | Time of measurement |
| start_weight | float | not null | The amount of water before adding water |
| water_added | float | defualt null | The amount of water added (should this be a criteria? What if the node was not able to send this data?) |

### Note:
Having both start weight and waterAdded maybe unnecessary, as they are coupled.

## Images

| name | type | constraints | comment |
| ---- | ---- | ---- | ------- |
| id | int | auto, **primery key** | - |
| measurement_id | int | forefin key, refernces measurments(id), ON DELETE CASCADE | refernce to the measurment id (1 to 1) |
| image_data | blob | defualt null | The image | 

## Impliment in sqlite

### Series
```SQL
CREATE TABLE Series (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
);
```

### Plants
```SQL
CREATE TABLE Plants (
    id INTEGER PRIMARY KEY,
    serie_id INTEGER NOT NULL,
    qr_code TEXT UNIQUE DEFAULT NULL,
    target_weight FLOAT NOT NULL,
    plant_type TEXT DEFAULT NULL,
    FOREIGN KEY (serie_id) REFERENCES Series(id) ON DELETE CASCADE
);
```

### Measurements
```SQL
CREATE TABLE Measurements (
    id INTEGER PRIMARY KEY,
    plant_id INTEGER NOT NULL,
    measured_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    start_weight FLOAT NOT NULL,
    water_added FLOAT DEFAULT NULL,
    FOREIGN KEY (plant_id) REFERENCES Plants(id) ON DELETE CASCADE
);
```

### Images
```SQl
CREATE TABLE Images (
    id INTEGER PRIMARY KEY,
    measurement_id INTEGER NOT NULL,
    image_data BLOB DEFAULT NULL,
    FOREIGN KEY (measurement_id) REFERENCES Measurements(id) ON DELETE CASCADE
);
```

### Nodes
```SQL
CREATE TABLE Nodes (
    id INTEGER PRIMARY KEY,
    mac TEXT NOT NULL,
    roles TEXT NOT NULL
);
```


## Notes on database:
1. 
Here I have set up sush that deleting a series deletes the plants related to that series, which deletes the measurements and images related to that plant(s).

Similarly, deleting a plant deletes the plants measurement, (but not the series of course).

## Notes related to sqlite:

1. 
In SQLite:
```SQL
id INTEGER PRIMARY KEY
```
automatically generates an integer ID. We do not need `AUTOINCREMENT`.

2. 
SQLite does not enforce foreign key constraints by default. This needs to be executed when establishing a database connection:
```SQL
PRAGMA foreign_keys = ON;
```

Python exaple:
```python
import sqlite3

conn = sqlite3.connect("plants.db")

# Enable foreign key constraints
conn.execute("PRAGMA foreign_keys = ON;")

# Now execute SQL queries
cursor = conn.cursor()
```