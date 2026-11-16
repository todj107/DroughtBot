import {create, update, deletes, read} from "./api.js";

function handle_data(fields){
    const data = {};
    for (const [key, { input, convert }] of Object.entries(fields)) {
        const value = input.value.trim();
        if (value === "") continue;
        data[key] = convert ? convert(value) : value;
    }

    return data;
}

async function get_plants() {
    const seriesId = new URLSearchParams(window.location.hash.slice(1)).get("serie_id");
    if (!seriesId) {
        response.textContent = "No Serie ID choosen";
    }

    const results = await read(`http://127.0.0.1:8000/series/${encodeURIComponent(seriesId)}/plants/`);

    const list = document.getElementById("item_list");
    list.innerHTML = "";

    // read() returns null on error
    if (!results) {
        list.textContent = "No Measurements found.";
        return;
    }

    // for (const result of results) {
    //     const link = document.createElement("a");
    //     link.className = "child";
    //     link.href = `./measurement.html#plant_id=${result.id}`;
    //     link.textContent = `ID: ${result.id} | Serie ID: ${result.serie_id} | QR-Code: ${result.qr_code} | Target weight: ${result.target_weight} | Plant type: ${result.plant_type}`;
    //     list.appendChild(link);
    // }

    const table = document.createElement("table");

    const headRow = table.createTHead().insertRow();
    for (const label of ["ID", "Serie ID", "QR-Code", "Target weight", "Plant type"]) {
        const th = document.createElement("th");
        th.textContent = label;
        headRow.appendChild(th);
    }

    const tbody = table.createTBody();
    for (const result of results) {
        const row = tbody.insertRow();

        const nameCell = row.insertCell();
        const link = document.createElement("a");
        link.href = `./measurement.html#plant_id=${result.id}`;
        link.textContent = result.id;
        nameCell.appendChild(link);

        row.insertCell().textContent = result.serie_id;
        row.insertCell().textContent = result.qr_code ?? "-";
        row.insertCell().textContent = result.target_weight;
        row.insertCell().textContent = result.plant_type ?? "-";
    }

    list.appendChild(table);
}

async function create_plant() {
    const response = document.getElementById("response");

    const seriesId = new URLSearchParams(window.location.hash.slice(1)).get("serie_id");
    if (!seriesId) {
        response.textContent = "No Serie ID choosen";
    }

    const fields = {
        qr_code: { input: document.getElementById("create_qrcode") },
        target_weight: { input: document.getElementById("create_target_weight"), convert: Number },
        plant_type: { input: document.getElementById("create_plant_type") },
    };

    const data = handle_data(fields);
    data.serie_id = Number(seriesId);
    if (data.target_weight === undefined) {
        response.textContent = "Target_weight must be set!";
        return;
    }

    const amount = Number(document.getElementById("create_amount").value.trim()) || 1;
    if (amount > 1 && data.qr_code) {
        response.textContent = "QR codes must be unique, so leave it empty when creating several plants";
        return;
    }

    let created = 0;
    for (let i = 0; i < amount; i++) {
        const result = await create("http://127.0.0.1:8000/plants/", data);
        if (!result) break;
        created++;
    }

    response.textContent = created === amount
        ? `${created} plant(s) successfully created`
        : `Created ${created} of ${amount} plants, then an error occurred`;
      
    document.getElementById("create_plant").reset();
    get_plants();
}

async function update_plant() {
    const response = document.getElementById("response");

    const id = document.getElementById("update_plant_id").value.trim();
    if (!id) {
        response.textContent = "Please enter a plant id";
        return;
    }

    // Map each JSON key to its input and an optional converter
    const fields = {
        serie_id: { input: document.getElementById("update_serie_id"), convert: Number },
        qr_code: { input: document.getElementById("update_qrcode") },
        target_weight: { input: document.getElementById("update_target_weight"), convert: Number },
        plant_type: { input: document.getElementById("update_plant_type") },
    };

    // Only include fields the user actually filled in
    const data = handle_data(fields);

    if (Object.keys(data).length === 0) {
        response.textContent = "Enter at least one field to update";
        return;
    }

    const result = await update(`http://127.0.0.1:8000/plants/${encodeURIComponent(id)}`, data);
    if (!result) {
        response.textContent = "Could not update Plant";
        return;
    }

    response.textContent = "Plant successfully updated";
    document.getElementById("update_plant").reset();
    
    get_plants();
}

async function delete_plant() {
    const id = document.getElementById("delete_id").value.trim();

    if (!id) {
        response.textContent = "Please enter a plant id";
        return;
    }

    const result = await deletes(`http://127.0.0.1:8000/plants/${encodeURIComponent(id)}`);

    if (!result) {
        response.textContent = "Could not delete Plant";
        return;
    }

    response.textContent = "Plant successfully deleted";
    document.getElementById("delete_plant").reset();
    get_plants();
}

// Runs directly (once)
const seriesId = new URLSearchParams(window.location.hash.slice(1)).get("serie_id");
document.getElementById("caption").textContent = `Plants in Series_id = ${seriesId}`;
get_plants();

// Runs when form is submitted
document.getElementById("create_plant").addEventListener("submit", (event) => {
    event.preventDefault();
    create_plant();
});

document.getElementById("update_plant").addEventListener("submit", (event) => {
    event.preventDefault();
    update_plant();
});

document.getElementById("delete_plant").addEventListener("submit", (event) => {
    event.preventDefault();
    delete_plant(); 
});