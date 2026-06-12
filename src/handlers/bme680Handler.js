import { saveEvent } from "../services/eventService.js";
import { decrypt } from "../crypto.js";

export function handleBme680(message) {

    const data = JSON.parse(decrypt(message.trim()));

    saveEvent(data.deviceId, "bme680", data);

    console.log(`[BME680] temp:${data.temp} hum:${data.hum} press:${data.press} gas:${data.gas}`);
}
