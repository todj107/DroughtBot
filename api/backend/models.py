from sqlalchemy import Column, Integer, String, Float, ForeignKey, DateTime, LargeBinary, text
from sqlalchemy.sql import func
from database import Base

class Serie(Base):
    __tablename__ = "Series"

    id = Column(Integer, primary_key=True)
    name = Column(String, nullable=False)
    created_at = Column(DateTime(timezone=True), nullable=False, server_default=func.now())


class Plant(Base):
    __tablename__ = "Plants"

    id = Column(Integer, primary_key=True)
    serie_id = Column(Integer, ForeignKey("Series.id"), nullable=False)
    qr_code = Column(String, unique=True, nullable=True)
    target_weight = Column(Float, nullable=False)
    plant_type = Column(String, nullable=True)

class Measurement(Base):
    __tablename__ = "Measurements"

    id = Column(Integer, primary_key=True)
    plant_id = Column(Integer, ForeignKey("Plants.id"), nullable=False)
    measured_at = Column(DateTime(timezone=True), nullable=False, server_default=func.now())
    start_weight = Column(Float, nullable=False)
    water_added = Column(Float)

class Image(Base):
    __tablename__ = "Images"

    id = Column(Integer, primary_key=True)
    measurement_id = Column(Integer, ForeignKey("Measurements.id"), nullable=False)
    image_data = Column(LargeBinary, nullable=True)