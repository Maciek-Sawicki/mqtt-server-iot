import { Router } from "express";
import { db } from "../db/database.js";

const router = Router();

const getByType = db.prepare(`
    SELECT id, device_id, created_at, payload
    FROM events
    WHERE sensor_type = ?
    ORDER BY created_at DESC
    LIMIT 100
`);

router.get("/events", (req, res) => {
    try {
        const rows = db.prepare(`
            SELECT *
            FROM events
            ORDER BY created_at DESC
            LIMIT 100
        `).all();
        res.json(rows);
    } catch (err) {
        res.status(500).json(err);
    }
});

router.get("/rfid", (req, res) => {
    try {
        const rows = getByType.all("rfid").map(row => ({
            id: row.id,
            device_id: row.device_id,
            created_at: row.created_at,
            ...JSON.parse(row.payload),
        }));
        res.json(rows);
    } catch (err) {
        res.status(500).json(err);
    }
});

router.get("/temp", (req, res) => {
    try {
        const rows = getByType.all("bme680").map(row => ({
            id: row.id,
            device_id: row.device_id,
            created_at: row.created_at,
            ...JSON.parse(row.payload),
        }));
        res.json(rows);
    } catch (err) {
        res.status(500).json(err);
    }
});

export default router;