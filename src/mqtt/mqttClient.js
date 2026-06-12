import mqtt from "mqtt";
import { handleRFID } from "../handlers/rfidHandler.js";
import { handleMotion } from "../handlers/motionHandler.js";
import { handleBme680 } from "../handlers/bme680Handler.js";

const client = mqtt.connect(process.env.MQTT_URL);

const topics = ["iot/rfid", "iot/motion", "iot/bme680"];

client.on("connect", () => {
    console.log("MQTT connected");

    client.subscribe(topics, (err) => {
        if (err) {
            console.error("Subscribe error:", err);
        } else {
            console.log("Subscribed to", topics.join(", "));
        }
    });
});

client.on("message", (topic, payload) => {
    const message = payload.toString();
    if (topic === "iot/rfid") handleRFID(message);
    else if (topic === "iot/motion") handleMotion(message);
    else if (topic === "iot/bme680") handleBme680(message);
});

client.on("error", (err) => {
    console.error("MQTT ERROR:", err);
});

export default client;