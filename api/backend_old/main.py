from typing import Annotated
from fastapi import FastAPI, Depends, HTTPException, Query
from sqlalchemy.orm import Session

from database import get_db
from models import Serie, Plant, Measurement, Image
import schemas

app = FastAPI(title="DB API")

################### Series ##################################

@app.post("/series/", response_model=schemas.SerieResponse)
def create_serie(serie: schemas.SerieCreate, db: Session = Depends(get_db)):
    db_serie = Serie(**serie.model_dump())
    
    db.add(db_serie)
    db.commit()
    db.refresh(db_serie)    
    return db_serie

@app.get("/series/", response_model=list[schemas.SerieResponse])
def read_series(offset: int = 0, limit: Annotated[int, Query(le=100)] = 100, db: Session = Depends(get_db)):
    db_series = db.query(Serie).offset(offset).limit(limit).all()
    return db_series

# MAYBE UNNECESSARY
# @app.get("/series/{serie_id}", response_model=schemas.SerieResponse)
# def read_serie(serie_id: int, db: Session = Depends(get_db)):
#     db_serie = db.get(Serie, serie_id)
#     if not db_serie:
#         raise HTTPException(status_code=404, detail="Serie not found")
#     return db_serie

@app.patch("/series/{serie_id}", response_model=schemas.SerieResponse)
def update_serie(serie_id: int, serie_data: schemas.SerieUpdate, db: Session = Depends(get_db)):
    db_serie = db.get(Serie, serie_id)
    if not db_serie:
        raise HTTPException(status_code=404, detail="Serie not found")

    update_data = serie_data.model_dump(exclude_unset=True)

    for key, value in update_data.items():
        setattr(db_serie, key, value)

    db.commit()
    db.refresh(db_serie)

    return db_serie

@app.delete("/series/{serie_id}", status_code=204)
def delete_serie(serie_id: int, db: Session = Depends(get_db)):
    db_serie = db.get(Serie, serie_id)
    if not db_serie:
        raise HTTPException(status_code=404, detail="Serie not found")
    db.delete(db_serie)
    db.commit()
    return None


################### Plants #####################################

@app.post("/plants/", response_model=schemas.PlantResponse)
def create_plant(plant: schemas.PlantCreate, db: Session = Depends(get_db)):
    db_plant = Plant(**plant.model_dump())
    db.add(db_plant)
    db.commit()
    db.refresh(db_plant)
    return db_plant


@app.get("/series/{serie_id}/plants", response_model=list[schemas.PlantResponse])
def read_plants_from_serie(serie_id: int, db: Session = Depends(get_db)):
    db_plants = db.query(Plant).filter(Plant.serie_id == serie_id).all()

    if not db_plants:
        raise HTTPException(status_code=404, detail="Plant(s) not found")
    return db_plants


@app.patch("/plants/{plant_id}", response_model=schemas.PlantResponse)
def update_plant(plant_id: int, plant_data: schemas.PlantUpdate, db: Session = Depends(get_db)):
    db_plant = db.get(Plant, plant_id)
    if not db_plant:
        raise HTTPException(status_code=404, detail="Plant not found")

    update_data = plant_data.model_dump(exclude_unset=True)

    for key, value in update_data.items():
        setattr(db_plant, key, value)

    db.commit()
    db.refresh(db_plant)

    return db_plant

@app.delete("/plants/{plant_id}", status_code=204)
def delete_plant(plant_id: int, db: Session = Depends(get_db)):
    db_plant = db.get(Plant, plant_id)
    if not db_plant:
        raise HTTPException(status_code=404, detail="Plant not found")
    db.delete(db_plant)
    db.commit()
    return None



###################### MEASUREMENTS #####################################


@app.post("/measurements/", response_model=schemas.MeasurementResponse)
def create_measurement(measurement: schemas.MeasurementCreate, db: Session = Depends(get_db)):
    db_measurement = Measurement(**measurement.model_dump())
    db.add(db_measurement)
    db.commit()
    db.refresh(db_measurement)
    return db_measurement


@app.get("/plants/{plant_id}/measurements/", response_model=list[schemas.MeasurementResponse])
def read_measurements_from_plant(plant_id: int, db: Session = Depends(get_db)):
    db_measurements = db.query(Measurement).filter(Measurement.plant_id == plant_id).all()

    if not db_measurements:
        raise HTTPException(status_code=404, detail="Measurement(s) not found")
    return db_measurements


@app.patch("/measurements/{measurement_id}", response_model=schemas.MeasurementResponse)
def update_measurement(measurement_id: int, measurement_data: schemas.MeasurementUpdate, db: Session = Depends(get_db)):
    db_measurement = db.get(Measurement, measurement_id)
    if not db_measurement:
        raise HTTPException(status_code=404, detail="Measurement not found")

    update_data = measurement_data.model_dump(exclude_unset=True)

    for key, value in update_data.items():
        setattr(db_measurement, key, value)

    db.commit()
    db.refresh(db_measurement)

    return db_measurement

@app.delete("/measurements/{measurement_id}", status_code=204)
def delete_measurement(measurement_id: int, db: Session = Depends(get_db)):
    db_measurement = db.get(Measurement, measurement_id)
    if not db_measurement:
        raise HTTPException(status_code=404, detail="Measurement not found")
    db.delete(db_measurement)
    db.commit()
    return None

