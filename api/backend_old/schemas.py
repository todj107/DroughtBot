from sqlalchemy import LargeBinary
from pydantic import BaseModel, ConfigDict
from datetime import datetime

#### Series
class SerieCreate(BaseModel):
    name: str

class SerieResponse(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    id: int
    name: str
    created_at: datetime

class SerieUpdate(BaseModel):
    name: str | None = None
    created_at: datetime | None = None


#### Plants
class PlantCreate(BaseModel):
    serie_id: int
    qr_code: str | None = None
    target_weight: float
    plant_type: str | None = None

class PlantResponse(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    id: int
    serie_id: int
    qr_code: str | None
    target_weight: float
    plant_type: str | None

class PlantUpdate(BaseModel):
    serie_id: int | None = None
    qr_code: str | None = None
    target_weight: float | None = None
    plant_type: str | None = None

#### Measurements
class MeasurementCreate(BaseModel):
    plant_id: int
    start_weight: float
    water_added: float | None

class MeasurementResponse(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    id: int
    plant_id: int
    measured_at: datetime
    start_weight: float
    water_added: float | None

class MeasurementUpdate(BaseModel):
    plant_id: int | None = None
    measured_at: datetime | None = None
    start_weight: float | None = None
    water_added: float | None = None

#### Images
class ImageCreate(BaseModel):
    measurement_id: int
    image_data: bytes

class ImageResponse(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    id: int
    measurement_id: int
    image_data: bytes

class ImageUpdate(BaseModel):
    measurement_id: int | None = None
    image_data: bytes | None = None