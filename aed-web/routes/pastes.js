const { Router } = require('express');
const { nanoid } = require('nanoid');
const pool = require('../db/pool');
const { autenticar } = require('../middleware/auth');

const router = Router();

// Crear paste
router.post('/', autenticar, async (req, res) => {
  try {
    const { nivel_id, content } = req.body;
    if (!nivel_id || !content) {
      return res.status(400).json({ error: 'nivel_id y content son obligatorios' });
    }

    const id = nanoid(8);
    await pool.query(
      'INSERT INTO pastes (id, user_id, nivel_id, content) VALUES ($1, $2, $3, $4)',
      [id, req.user.id, nivel_id, content]
    );

    res.status(201).json({ id, url: `/p/${id}` });
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Error interno' });
  }
});

// Ver paste individual
router.get('/:id', async (req, res) => {
  try {
    const { rows } = await pool.query(
      `SELECT p.*, u.nombre, u.seccion, n.titulo AS nivel_titulo, n.numero AS nivel_numero
       FROM pastes p
       JOIN users u ON p.user_id = u.id
       JOIN niveles n ON p.nivel_id = n.id
       WHERE p.id = $1`,
      [req.params.id]
    );

    if (rows.length === 0) {
      return res.status(404).json({ error: 'Paste no encontrado' });
    }

    res.json(rows[0]);
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Error interno' });
  }
});

// Mis pastes
router.get('/', autenticar, async (req, res) => {
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

module.exports = router;
