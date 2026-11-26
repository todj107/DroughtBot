const params = new URLSearchParams(window.location.search);
const measurementId = params.get("image_id");
const response = document.getElementById("response");

if (!imageId) {
    response.textContent = "No image_id in URL (?measurement_id=1)";
} else {
    document.getElementById("caption").textContent = `Image (measurement_id: ${measurementId})`;

    // Fetch JSON to show base64 raw data
    fetch(`/images/${encodeURIComponent(imageId)}`)
        .then(r => r.ok ? r.json() : Promise.reject(r.status))
        .then(data => {
            document.getElementById("raw_data").value = data.image_data;
        })
        .catch(err => {
            response.textContent = `Could not load image ${imageId}: ${err}`;
        });

    const results = await read(`/measurements/${measurementId}/images`);
    // read() returns null on error
    if (!results) {
        response.textContent = "No Image found.";
        return;
    }

    const fileUrl = results.data;

    // Render as image and link using the /file endpoint directly
    document.getElementById("image_display").src = fileUrl;
    const link = document.getElementById("image_link");
    link.href = fileUrl;
    link.textContent = "Download raw";
}
