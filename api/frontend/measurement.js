import {create, update, deletes, read} from "./api.js";

async function get_measurements() {
    const plantId = new URLSearchParams(window.location.hash.slice(1)).get("plant_id");
    if (!plantId) {
        response.textContent = "No Plant ID choosen";
    }

    const results = await read(`/plants/${encodeURIComponent(plantId)}/measurements`);

    const list = document.getElementById("item_list");
    list.innerHTML = "";

    // read() returns null on error
    if (!results) {
        list.textContent = "No Measurements found.";
        return;
    }

    const table = document.createElement("table");

    const headRow = table.createTHead().insertRow();
    for (const label of ["Plant ID", "ID", "Start weight", "Water added", "Measured at"]) {
        const th = document.createElement("th");
        th.textContent = label;
        headRow.appendChild(th);
    }

    const tbody = table.createTBody();
    for (const result of results) {
        const row = tbody.insertRow();

        row.insertCell().textContent = result.plant_id;

        const nameCell = row.insertCell();
        const link = document.createElement("a");
        link.href = `./image.html?measurement_id=${result.id}`;
        link.textContent = result.id;
        nameCell.appendChild(link);

        row.insertCell().textContent = result.start_weight ?? "-";
        row.insertCell().textContent = result.water_added ?? "-";
        row.insertCell().textContent = result.measured_at ?? "-";
    }

    list.appendChild(table);
}


async function delete_measurement() {
    const id = document.getElementById("delete_id").value.trim();

    if (!id) {
        response.textContent = "Please enter a measurement id";
        return;
    }

    const result = await deletes(`/measurements/${encodeURIComponent(id)}`);

    if (!result) {
        response.textContent = "Could not delete measurement";
        return;
    }

    response.textContent = "Measurement successfully deleted";
    document.getElementById("delete_measurement").reset();
    get_measurements();
}

// Runs directly (once)
const plantId = new URLSearchParams(window.location.hash.slice(1)).get("plant_id");
document.getElementById("caption").textContent = `Measuremnts in Plant_id = ${plantId}`;
get_measurements();

document.getElementById("delete_measurement").addEventListener("submit", (event) => {
    event.preventDefault();
    delete_measurement(); 
});