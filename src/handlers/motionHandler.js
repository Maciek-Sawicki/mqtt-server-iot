import { saveEvent } from "../services/eventService.js";
import { decrypt } from "../crypto.js";

export function handleMotion(message) {

    const data = JSON.parse(decrypt(message.trim()));

    saveEvent(
        data.deviceId,
        "motion",
        data
    );

    console.log(
        `[MOTION] ${data.motion}`
    );
}