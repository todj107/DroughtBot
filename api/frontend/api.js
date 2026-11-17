async function create_or_update(method, url, data) {
    try {
        const response = await fetch(url, {
            method: method,
            headers: {
                "Content-Type": "application/json"
            },
            body: JSON.stringify(data)
        });

        const result = await response.json().catch(() => null);
        if (!response.ok) {
            throw new Error(result.detail || "Something went wrong");
        }

        return result;
    } catch (error) {
        console.error("Error:", error);
        return null;
    }
}

async function read_or_delete(method, url) {
    try {
        const response = await fetch(url, {method: method});
        const result = await response.json().catch(() => null);  

        if (!response.ok) {
            throw new Error(result.detail || "Something went wrong");
        }

        if (method.toUpperCase() === "DELETE") {
            return true;
        }

        return result;

    } catch (error) {
        console.error("Error:", error);
        return null;
    }
}

export function create(url, data){return create_or_update("POST", url, data);}
export function update(url, data){return create_or_update("PATCH", url, data);}
export function read(url){return read_or_delete("GET", url);}
export function deletes(url){return read_or_delete("DELETE", url);}