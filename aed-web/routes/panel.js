const { Router } = require('express');
const pool = require('../db/pool');
const { autenticar, soloProfe } = require('../middleware/auth');

const router = Router();

router.use(autenticar, soloProfe);

// Listar entregas con filtros
router.get('/entregas', async (req, res) => {
  try {
    const { seccion, nivel, estado } = req.query;
    let query = `
      SELECT p.id, p.content, p.estado, p.nota_profe, p.created_at,
             u.nombre, u.email, u.seccion,
             n.titulo AS nivel_titulo, n.numero AS nivel_numero
      FROM pastes p
      JOIN users u ON p.user_id = u.id
      JOIN niveles n ON p.nivel_id = n.id
      WHERE 1=1
    `;
    const params = [];

    if (seccion) {
      params.push(seccion);
      query += ` AND u.seccion = $${params.length}`;
    }
    if (nivel) {
      params.push(parseInt(nivel));
      query += ` AND n.numero = $${params.length}`;
    }
    if (estado) {
      params.push(estado);
      query += ` AND p.estado = $${params.length}`;
    }

    query += ' ORDER BY p.created_at DESC';

    const { rows } = await pool.query(query, params);
    res.json(rows);
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Error interno' });
  }
});

// Corregir individual
router.patch('/entregas/:id', async (req, res) => {
  try {
    const { estado, nota_profe } = req.body;
    if (!estado) {
      return res.status(400).json({ error: 'estado es obligatorio' });
    }

    const { rows } = await pool.query(
      'UPDATE pastes SET estado = $1, nota_profe = $2 WHERE id = $3 RETURNING *',
      [estado, nota_profe || null, req.params.id]
    );

    if (rows.length === 0) {
      return res.status(404).json({ error: 'Entrega no encontrada' });
    }

    res.json(rows[0]);
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Error interno' });
  }
});

// Corregir en lote
router.post('/corregir-lote', async (req, res) => {
  try {
    const items = req.body;
    if (!Array.isArray(items) || items.length === 0) {
      return res.status(400).json({ error: 'Se espera un array de correcciones' });
    }

    const results = [];
    for (const item of items) {
      const { id, estado, nota_profe } = item;
      const { rows } = await pool.query(
        'UPDATE pastes SET estado = $1, nota_profe = $2 WHERE id = $3 RETURNING id, estado',
        [estado, nota_profe || null, id]
      );
      if (rows.length > 0) results.push(rows[0]);
    }

    res.json({ actualizados: results.length, results });
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Error interno' });
  }
});

module.exports = router;
