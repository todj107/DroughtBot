import {read} from "./api.js";

const params = new URLSearchParams(window.location.search);
const measurementId = params.get("measurement_id");
const response = document.getElementById("response");

if (!measurementId) {
    response.textContent = "No measurement_id in URL (?measurement_id=1)";
} else {
    document.getElementById("caption").textContent = `Image (measurement_id: ${measurementId})`;

    // read() returns null on error
    const results = await read(`/measurements/${encodeURIComponent(measurementId)}/images`);
    const image = results[0];
    console.log(image.id);
    if (!image) {
        response.textContent = "No image found.";
    } else {
        const fileUrl = `/images/${image.id}/file`;

        document.getElementById("image_display").src = fileUrl;
        const link = document.getElementById("image_link");
        link.href = fileUrl;
        link.textContent = "Download raw";

        document.getElementById("raw_data").textContent = image.image_data;
    }
}
