"""
Run info

Required packages:
    opencv-python-headless numpy

Rund code:
    python analyse_plant.py plant.jpg
    python analyse_plant.py < plant.jpg

Output:
    {
    "plant_area_cm2": 20.05,
    "mean_exg": 0.4821
    }
"""
    
  
import argparse
import json
import sys

import cv2
import numpy as np

MARKER_SIZE = 20.0  # mm


def clean_mask(mask):
    """Remove small objects and fill small gaps."""
    kernel = np.ones((3, 3), np.uint8)
    mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel)
    return cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel)


def calibrate(hsv):
    """Detect the red square and return mm² per pixel."""
    mask1 = cv2.inRange(hsv, (0, 100, 70), (10, 255, 255))
    mask2 = cv2.inRange(hsv, (170, 100, 70), (179, 255, 255))
    mask = clean_mask(cv2.bitwise_or(mask1, mask2))

    contours, _ = cv2.findContours(
        mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE
    )
    markers = []

    for contour in contours:
        # Reject small objects
        if cv2.contourArea(contour) < 100:
            continue

        # Require four corners and a convex shape
        perimeter = cv2.arcLength(contour, True)
        corners = cv2.approxPolyDP(contour, 0.02 * perimeter, True)
        if len(corners) != 4 or not cv2.isContourConvex(corners):
            continue

        # Require approximately equal width and height
        _, (width, height), _ = cv2.minAreaRect(contour)
        if max(width, height) == 0 or min(width, height) / max(width, height) < 0.9:
            continue

        markers.append(corners)

    # Reject missing or ambiguous calibration markers
    if len(markers) != 1:
        raise ValueError(f"Expected one calibration marker, found {len(markers)}")

    pixel_area = cv2.contourArea(markers[0])
    if pixel_area <= 0:
        raise ValueError("Invalid calibration marker area")

    return MARKER_SIZE**2 / pixel_area


def segment_plant(hsv):
    """Segment green pixels and keep the largest connected object."""
    mask = cv2.inRange(hsv, (25, 40, 40), (95, 255, 255))
    mask = clean_mask(mask)

    num_labels, labels, stats, _ = cv2.connectedComponentsWithStats(
        mask, connectivity=8
    )
    if num_labels < 2:
        raise ValueError("No plant detected")

    largest_label = 1 + np.argmax(stats[1:, cv2.CC_STAT_AREA])
    plant_mask = np.zeros_like(mask)
    plant_mask[labels == largest_label] = 255
    return plant_mask


def measure_plant(img):
    """Calculate projected plant area and mean normalized Excess Green."""
    hsv = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)
    img_rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)

    mm2_per_pixel = calibrate(hsv)
    plant_mask = segment_plant(hsv)

    # Convert segmented pixel count to square centimetres
    plant_area_px = cv2.countNonZero(plant_mask)
    plant_area_cm2 = plant_area_px * mm2_per_pixel / 100

    # Extract and normalize the RGB values of plant pixels
    pixels = img_rgb[plant_mask > 0].astype(float)
    r, g, b = pixels.T
    total = r + g + b + 1e-8
    r, g, b = r / total, g / total, b / total

    # Calculate mean Excess Green
    mean_exg = np.mean(2 * g - r - b)

    return {
        "plant_area_cm2": float(plant_area_cm2),
        "mean_exg": float(mean_exg),
    }


def main():
    parser = argparse.ArgumentParser(description="Measure a plant from an image.")
    parser.add_argument("image", nargs="?", help="Image path; omit to read raw image bytes from stdin")
    args = parser.parse_args()

    try:
        # Load an image file or decode raw bytes from the database
        if args.image:
            img = cv2.imread(args.image)
        else:
            blob = sys.stdin.buffer.read()
            if not blob:
                raise ValueError("No image data received")
            img = cv2.imdecode(np.frombuffer(blob, dtype=np.uint8), cv2.IMREAD_COLOR)

        if img is None:
            raise ValueError("Could not decode image")

        # stdout contains only JSON on success
        print(json.dumps(measure_plant(img), allow_nan=False))
        return 0

    except (ValueError, OSError, cv2.error) as exc:
        # Send errors to stderr and return a non-zero exit code
        print(str(exc), file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())

