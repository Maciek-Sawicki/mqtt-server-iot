import "dotenv/config";

import express from "express";

import "./mqtt/mqttClient.js";

import apiRoutes from "./routes/api.js";

import { runMigrations } from "./db/migrations.js";

runMigrations();

const app = express();

app.use(express.json());
app.use(express.static("public"));

app.use("/api", apiRoutes);

const port = process.env.PORT || 3000;

app.listen(port, () => {
    console.log(
        `Server started on ${port}`
    );
});