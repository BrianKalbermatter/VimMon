const express = require('express');
const path = require('path');

const pool = require('./db/pool');
const { autenticar } = require('./middleware/auth');
const authRoutes = require('./routes/auth');
const pastesRoutes = require('./routes/pastes');
const panelRoutes = require('./routes/panel');

const app = express();
const PORT = process.env.PORT || 3000;

app.use(express.json());
app.use(express.static(path.join(__dirname, 'public')));

// API
app.use('/api', authRoutes);
app.use('/api/pastes', pastesRoutes);

// Mis entregas
app.get('/api/mis-pastes', autenticar, async (req, res) => {
  try {
    const { rows } = await pool.query(
      `SELECT p.id, p.nivel_id, p.estado, p.nota_profe, p.created_at, n.titulo AS nivel_titulo, n.numero AS nivel_numero
       FROM pastes p
       JOIN niveles n ON p.nivel_id = n.id
       WHERE p.user_id = $1
       ORDER BY p.created_at DESC`,
      [req.user.id]
    );
    res.json(rows);
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Error interno' });
  }
});
app.use('/api/panel', panelRoutes);

// Niveles API
app.get('/api/niveles', async (req, res) => {
  try {
    const { rows } = await pool.query('SELECT * FROM niveles ORDER BY numero');
    res.json(rows);
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Error interno' });
  }
});

// SPA-style routes for HTML pages
app.get('/niveles', (req, res) => res.sendFile(path.join(__dirname, 'public', 'niveles.html')));
app.get('/p/:id', (req, res) => res.sendFile(path.join(__dirname, 'public', 'paste.html')));
app.get('/panel', (req, res) => res.sendFile(path.join(__dirname, 'public', 'panel.html')));

app.listen(PORT, () => {
  console.log(`AED Web corriendo en http://localhost:${PORT}`);
});
