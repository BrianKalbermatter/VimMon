const { Router } = require('express');
const bcrypt = require('bcrypt');
const jwt = require('jsonwebtoken');
const pool = require('../db/pool');
const { SECRET } = require('../middleware/auth');

const router = Router();

router.post('/register', async (req, res) => {
  try {
    const { email, password, nombre, seccion, role } = req.body;
    if (!email || !password || !nombre) {
      return res.status(400).json({ error: 'email, password y nombre son obligatorios' });
    }

    const hash = await bcrypt.hash(password, 10);
    const userRole = role === 'profe' ? 'profe' : 'alumno';

    const { rows } = await pool.query(
      'INSERT INTO users (email, password_hash, nombre, role, seccion) VALUES ($1, $2, $3, $4, $5) RETURNING id, email, nombre, role, seccion',
      [email, hash, nombre, userRole, seccion || null]
    );

    const user = rows[0];
    const token = jwt.sign(
      { id: user.id, email: user.email, nombre: user.nombre, role: user.role, seccion: user.seccion },
      SECRET,
      { expiresIn: '7d' }
    );

    res.status(201).json({ user, token });
  } catch (err) {
    if (err.code === '23505') {
      return res.status(409).json({ error: 'Email ya registrado' });
    }
    console.error(err);
    res.status(500).json({ error: 'Error interno' });
  }
});

router.post('/login', async (req, res) => {
  try {
    const { email, password } = req.body;
    if (!email || !password) {
      return res.status(400).json({ error: 'email y password son obligatorios' });
    }

    const { rows } = await pool.query('SELECT * FROM users WHERE email = $1', [email]);
    if (rows.length === 0) {
      return res.status(401).json({ error: 'Credenciales inválidas' });
    }

    const user = rows[0];
    const valid = await bcrypt.compare(password, user.password_hash);
    if (!valid) {
      return res.status(401).json({ error: 'Credenciales inválidas' });
    }

    const token = jwt.sign(
      { id: user.id, email: user.email, nombre: user.nombre, role: user.role, seccion: user.seccion },
      SECRET,
      { expiresIn: '7d' }
    );

    res.json({
      user: { id: user.id, email: user.email, nombre: user.nombre, role: user.role, seccion: user.seccion },
      token
    });
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Error interno' });
  }
});

module.exports = router;
