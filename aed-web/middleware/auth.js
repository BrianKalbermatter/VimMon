const jwt = require('jsonwebtoken');

const SECRET = process.env.JWT_SECRET || 'cambiar_en_produccion';

function autenticar(req, res, next) {
  const header = req.headers.authorization;
  if (!header) return res.status(401).json({ error: 'Token requerido' });

  const token = header.replace('Bearer ', '');
  try {
    req.user = jwt.verify(token, SECRET);
    next();
  } catch {
    res.status(401).json({ error: 'Token inválido' });
  }
}

function soloProfe(req, res, next) {
  if (req.user.role !== 'profe') {
    return res.status(403).json({ error: 'Solo profesores' });
  }
  next();
}

module.exports = { autenticar, soloProfe, SECRET };
