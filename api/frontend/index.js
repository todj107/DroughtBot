import {create, update, deletes, read} from "./api.js";

async function get_series() {
    const results = await read("http://127.0.0.1:8000/series/");

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
    //     link.href = `./plant.html#serie_id=${result.id}`;
    //     link.textContent = `ID: ${result.id} | Name: ${result.name} | Created at: ${result.created_at}`;
    //     list.appendChild(link);
    // }

    const table = document.createElement("table");

    const headRow = table.createTHead().insertRow();
    for (const label of ["ID", "Name", "Created at"]) {
        const th = document.createElement("th");
        th.textContent = label;
        headRow.appendChild(th);
    }

    const tbody = table.createTBody();
    for (const result of results) {
        const row = tbody.insertRow();

        row.insertCell().textContent = result.id;

        const nameCell = row.insertCell();
        const link = document.createElement("a");
        link.href = `./plant.html#serie_id=${result.id}`;
        link.textContent = result.name;
        nameCell.appendChild(link);

        row.insertCell().textContent = new Date(result.created_at).toLocaleString();
    }

    list.appendChild(table);
}

async function create_serie() {
    const input = document.getElementById("create_name");
    const response = document.getElementById("response");

    const name = input.value.trim();
    if (!name) {
        response.textContent = "Please enter a name";
        return;
    }

    const result = await create("http://127.0.0.1:8000/series/", { name });
    if (!result) {
        response.textContent = "Could not create Serie";
        return;
    }
    
    response.textContent = "Series successfully created";    
    input.value = "";
    get_series();
}

async function update_serie() {
    const id = document.getElementById("update_id").value.trim();
    const name = document.getElementById("update_name").value.trim();
    const response = document.getElementById("response");

    if (!name || !id) {
        response.textContent = "Please enter a name and id";
        return;
    }

    const result = await update(`http://127.0.0.1:8000/series/${encodeURIComponent(id)}`, { name });

    if (!result) {
        response.textContent = "Could not update Serie";
        return;
    }

    response.textContent = "Series successfully updated";
    document.getElementById("update_serie").reset();
    get_series();
}

async function delete_serie() {
    const idInput = document.getElementById("delete_id");

    const id = idInput.value.trim();

    if (!id) {
        response.textContent = "Please enter a id";
        return;
    }

    const result = await deletes(`http://127.0.0.1:8000/series/${encodeURIComponent(id)}`);

    if (!result) {
        response.textContent = "Could not delete Serie";
        return;
    }

    response.textContent = "Series successfully deleted";
    idInput.value = "";
    get_series();
}

// Runs directly (once)
get_series();
document.getElementById("caption").textContent = "Series"

// Runs when form is submitted
document.getElementById("create_serie").addEventListener("submit", (event) => {
    event.preventDefault();
    create_serie();
});

document.getElementById("update_serie").addEventListener("submit", (event) => {
    event.preventDefault();
    update_serie();
});

document.getElementById("delete_serie").addEventListener("submit", (event) => {
    event.preventDefault();
    delete_serie();
});