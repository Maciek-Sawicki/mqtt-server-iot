import { saveEvent } from "../services/eventService.js";
import { decrypt } from "../crypto.js";

export function handleRFID(message) {

    const data = JSON.parse(decrypt(message.trim()));
    const payload = { uid: data.uid, scannedAt: new Date().toISOString() };

    saveEvent(data.deviceId, "rfid", payload);

    console.log(`[RFID] ${data.deviceId} -> ${data.uid}`);
}