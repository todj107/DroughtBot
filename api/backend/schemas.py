from pydantic import BaseModel, ConfigDict
from datetime import datetime

#### Series
class SerieBase(BaseModel):
    name: str

class SerieOut(SerieBase):
    id: int
    created_at: datetime

    model_config = ConfigDict(from_attributes=True)

class SerieUpdate(BaseModel):
    name: str | None = None

#### Plants
class PlantBase(BaseModel):
    serie_id: int
    qr_code: str | None = None
    target_weight: float
    plant_type: str | None = None

class PlantOut(PlantBase):
    id: int
    model_config = ConfigDict(from_attributes=True)

class PlantUpdate(BaseModel):
    serie_id: int | None = None
    qr_code: str | None = None
    target_weight: float | None = None
    plant_type: str | None = None


#### Measurements
class MeasurementBase(BaseModel):
    plant_id: int
    start_weight: float
    water_added: float | None

class MeasurementsOut(MeasurementBase):
    id: int
    measured_at: datetime

    model_config = ConfigDict(from_attributes=True)

class MeasurementUpdate(BaseModel):
    plant_id: int | None = None
    start_weight: float | None = None
    water_added: float | None = None


#### Images
# image_data is sent/returned as a base64 string in JSON
class ImageBase(BaseModel):
    measurement_id: int
    image_data: bytes

    model_config = ConfigDict(val_json_bytes='base64', ser_json_bytes='base64')

class ImageOut(ImageBase):
    id: int

    model_config = ConfigDict(from_attributes=True, val_json_bytes='base64', ser_json_bytes='base64')

class ImageUpdate(BaseModel):
    measurement_id: int | None = None
    image_data: bytes | None = None

    model_config = ConfigDict(val_json_bytes='base64', ser_json_bytes='base64')