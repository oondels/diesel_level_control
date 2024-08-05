import cors from "cors";
import express from "express";
import http from "http";
import { WebSocketServer } from "ws";
import { pool } from "./db.cjs";

const app = express();
const server = http.createServer(app);
const wss = new WebSocketServer({ server });
const port = 2399;

app.use(cors());
app.use(express.json());

wss.on("connection", (ws) => {
  console.log("Socket connected");

  const sendDieselData = async () => {
    try {
      const query = await pool.query(`
        SELECT * FROM manutencao.diesel ORDER BY id DESC LIMIT 1;
      `);
      const result = query.rows[0];
      ws.send(JSON.stringify(result));
    } catch (error) {
      console.error("Erro ao buscar dados!", error);
    }
  };

  // Enviar dados a cada 5 segundos
  const intervalId = setInterval(sendDieselData, 5000);

  // Limpar intervalo quando a conexão é fechada
  ws.on("close", () => {
    clearInterval(intervalId);
    console.log("Socket disconnected");
  });
});

app.post("/post-diesel", async (req, res) => {
  try {
    // const result = await fetchLatestDieselData();
    const { nivel, unidade_dass, distance } = req.body;

    const postDiesel = await pool.query(
      `
        INSERT INTO manutencao.diesel (nivel, unidade_dass)
        VALUES ($1, $2);
      `,
      [nivel, unidade_dass]
    );

    return res.status(200).json({ response: "Dados Recebidos" });
  } catch (error) {
    console.error("Error:", error);
    return res.status(500).json({ response: "Erro ao consultar dados" });
  }
});

server.listen(port, () => {
  console.log(`Server listening on port ${port}`);
});
