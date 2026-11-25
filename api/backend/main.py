from fastapi import FastAPI, Depends, HTTPException, Query
from sqlalchemy.orm import Session
import models, schemas
from database import get_db
# from fastapi.middleware.cors import CORSMiddleware

from fastapi.staticfiles import StaticFiles
from pathlib import Path


app = FastAPI(title="DB API")

# app.add_middleware(
#     CORSMiddleware,
#     allow_origins=["http://127.0.0.1:5500", "http://localhost:5500"],
#     allow_methods=["*"],
#     allow_headers=["*"],
# )


################### Series ##################################
# Create a Serie
@app.post("/series/", response_model=schemas.SerieOut)
def create_serie(serie: schemas.SerieBase, db: Session = Depends(get_db)):
    db_serie = models.Serie(
        name=serie.name
    )
    db.add(db_serie)
    db.commit()
    db.refresh(db_serie)

    return db_serie

# Get multiple Series
@app.get("/series/", response_model=list[schemas.SerieOut])
def read_series(
            skip: int = 0, 
            limit: int = 100, 
            db: Session = Depends(get_db)
        ):
    db_series = db.query(models.Serie).offset(skip).limit(limit).all()
    return db_series

# Update a Serie
@app.patch("/series/{serie_id}", response_model=schemas.SerieOut)
def update_serie(
            serie_id: int, 
            serie_update: schemas.SerieUpdate, 
            db: Session = Depends(get_db)
        ):
    
    db_serie = db.get(models.Serie, serie_id)
    if not db_serie:
        raise HTTPException(status_code=404, detail="Serie not found")

    # Update fields
    update_data = serie_update.model_dump(exclude_unset=True)
    for key, value in update_data.items():
        setattr(db_serie, key, value)

    db.commit()
    db.refresh(db_serie)

    return db_serie


@app.delete("/series/{serie_id}", status_code=204)
def delete_serie(serie_id: int, db: Session = Depends(get_db)):
    db_serie = db.get(models.Serie, serie_id)
    if not db_serie:
        raise HTTPException(status_code=404, detail="Serie not found")
    
    db.delete(db_serie)
    db.commit()

    return None


################### Plants #####################################

@app.post("/plants/", response_model=schemas.PlantOut)
def create_plant(plant: schemas.PlantBase, db: Session = Depends(get_db)):
    # Check if qr_code already exsists
    if plant.qr_code is not None:
        db_user = db.query(models.Plant).filter(models.Plant.qr_code == plant.qr_code).first()
        if db_user:
            raise HTTPException(
                status_code=400,
                detail="QR-code already registered"
            )
    
    db_plant = models.Plant(**plant.model_dump())
    db.add(db_plant)
    db.commit()
    db.refresh(db_plant)

    return db_plant


@app.get("/series/{serie_id}/plants", response_model=list[schemas.PlantOut])
def read_plants_from_serie(serie_id: int, db: Session = Depends(get_db)):
    db_plants = db.query(models.Plant).filter(models.Plant.serie_id == serie_id).all()
    if not db_plants:
        raise HTTPException(status_code=404, detail="Plant(s) not found")
    
    return db_plants


@app.get("/plants/qr/{qr_code}", response_model=schemas.PlantOut)
def read_plant_from_qr(qr_code: str, db: Session = Depends(get_db)):
    db_plant = db.query(models.Plant).filter(models.Plant.qr_code == qr_code).first()
    if not db_plant:
        raise HTTPException(
            status_code=400,
            detail="QR-code not found"
        )

    return db_plant


@app.patch("/plants/{plant_id}", response_model=schemas.PlantOut)
def update_plant(
            plant_id: int, 
            plant_update: schemas.PlantUpdate, 
            db: Session = Depends(get_db)
        ):
    db_plant = db.get(models.Plant, plant_id)
    if not db_plant:
        raise HTTPException(status_code=404, detail="Plant not found")

    update_data = plant_update.model_dump(exclude_unset=True)
    for key, value in update_data.items():
        setattr(db_plant, key, value)

    db.commit()
    db.refresh(db_plant)

    return db_plant

@app.delete("/plants/{plant_id}", status_code=204)
def delete_plant(plant_id: int, db: Session = Depends(get_db)):
    db_plant = db.get(models.Plant, plant_id)
    if not db_plant:
        raise HTTPException(status_code=404, detail="Plant not found")
    
    db.delete(db_plant)
    db.commit()

    return None



###################### MEASUREMENTS #####################################

@app.post("/measurements/", response_model=schemas.MeasurementsOut)
def create_measurement(measurement: schemas.MeasurementBase, db: Session = Depends(get_db)):
    db_measurement = models.Measurement(**measurement.model_dump())
    db.add(db_measurement)
    db.commit()
    db.refresh(db_measurement)

    return db_measurement


@app.get("/plants/{plant_id}/measurements", response_model=list[schemas.MeasurementsOut])
def read_measurements_from_plant(plant_id: int, db: Session = Depends(get_db)):
    db_measurements = db.query(models.Measurement).filter(models.Measurement.plant_id == plant_id).all()
    if not db_measurements:
        raise HTTPException(status_code=404, detail="Measurement(s) not found")
    
    return db_measurements


@app.patch("/measurements/{measurement_id}", response_model=schemas.MeasurementsOut)
def update_measurement(
            measurement_id: int, 
            measurement_update: schemas.MeasurementUpdate, 
            db: Session = Depends(get_db)
        ):
    db_measurement = db.get(models.Measurement, measurement_id)
    if not db_measurement:
        raise HTTPException(status_code=404, detail="Measurement not found")

    update_data = measurement_update.model_dump(exclude_unset=True)
    for key, value in update_data.items():
        setattr(db_measurement, key, value)

    db.commit()
    db.refresh(db_measurement)

    return db_measurement

@app.delete("/measurements/{measurement_id}", status_code=204)
def delete_measurement(measurement_id: int, db: Session = Depends(get_db)):
    db_measurement = db.get(models.Measurement, measurement_id)
    if not db_measurement:
        raise HTTPException(status_code=404, detail="Measurement not found")
    
    db.delete(db_measurement)
    db.commit()

    return None


################### Image #####################################

@app.post("/images/", response_model=schemas.ImageOut)
def create_image(image: schemas.ImageBase, db: Session = Depends(get_db)):
    db_image = models.Image(**image.model_dump())

    db.add(db_image)
    db.commit()
    db.refresh(db_image)

    return db_image


@app.get("/measurements/{measurement_id}/images", response_model=list[schemas.ImageOut])
def read_images_from_measurement(measurement_id: int, db: Session = Depends(get_db)):
    db_images = db.query(models.Image).filter(models.Image.measurement_id == measurement_id).all()
    if not db_images:
        raise HTTPException(status_code=404, detail="Image(s) not found")
    
    return db_images


@app.patch("/images/{image_id}", response_model=schemas.ImageOut)
def update_image(
            image_id: int, 
            image_update: schemas.ImageUpdate, 
            db: Session = Depends(get_db)
        ):
    db_image = db.get(models.Image, image_id)
    if not db_image:
        raise HTTPException(status_code=404, detail="Image not found")

    update_data = image_update.model_dump(exclude_unset=True)
    for key, value in update_data.items():
        setattr(db_image, key, value)

    db.commit()
    db.refresh(db_image)

    return db_image

@app.delete("/images/{image_id}", status_code=204)
def delete_image(image_id: int, db: Session = Depends(get_db)):
    db_image = db.get(models.Image, image_id)
    if not db_image:
        raise HTTPException(status_code=404, detail="Image not found")
    
    db.delete(db_image)
    db.commit()

    return None


################# Node ##################
@app.post("/nodes", response_model=schemas.NodeBase)
def create_node(node: schemas.NodeBase, db: Session = Depends(get_db)):
    db_node = models.Node(**node.model_dump())

    db.add(db_node)
    db.commit()
    db.refresh(db_node)

    return db_node

@app.get("/nodes", response_model=list[schemas.NodeOut])
def read_nodes(
            skip: int = 0, 
            limit: int = 100, 
            db: Session = Depends(get_db)
        ):
    db_nodes = db.query(models.Node).offset(skip).limit(limit).all()
    return db_nodes



FRONTEND_DIR = Path(__file__).resolve().parent.parent / "frontend"
app.mount("/", StaticFiles(directory=FRONTEND_DIR, html=True), name="frontend")