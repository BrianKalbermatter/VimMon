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

// Niveles con progreso (autenticado)
app.get('/api/niveles-progreso', autenticar, async (req, res) => {
  try {
    const { rows: niveles } = await pool.query('SELECT * FROM niveles ORDER BY numero');
    // Niveles aprobados por el usuario
    const { rows: aprobados } = await pool.query(
      `SELECT DISTINCT nivel_id FROM pastes WHERE user_id = $1 AND estado = 'aprobado'`,
      [req.user.id]
    );
    const aprobadosSet = new Set(aprobados.map(r => r.nivel_id));

    const resultado = niveles.map((n, i) => {
      const aprobado = aprobadosSet.has(n.id);
      // Nivel 1 siempre desbloqueado; nivel N desbloqueado si nivel N-1 aprobado
      let desbloqueado = i === 0;
      if (i > 0) {
        desbloqueado = aprobadosSet.has(niveles[i - 1].id);
      }
      return { ...n, desbloqueado, aprobado };
    });

    res.json(resultado);
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Error interno' });
  }
});

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
