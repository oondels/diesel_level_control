import cors from "cors";
import express from "express";
import http from "http";

const app = express();
const port = 2399;

const server = http.createServer(app);

server.listen(port, () => console.log("Server running on port:", port));

app.use(cors());
app.use(express.json());

app.get("/diesel", (req, res) => {
  const level_test = req.query.diesel;
  console.log(level_test);

  res.json({ response: "Data received" });
});
