import { db } from "../db/database.js";

export function saveEvent(deviceId, sensorType, payload) {

    db.prepare(`
        INSERT INTO events
        (device_id, sensor_type, payload)
        VALUES (?, ?, ?)
    `).run(deviceId, sensorType, JSON.stringify(payload));
}