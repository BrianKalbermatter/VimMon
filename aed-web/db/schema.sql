CREATE TABLE users (
  id SERIAL PRIMARY KEY,
  email VARCHAR(255) UNIQUE NOT NULL,
  password_hash VARCHAR(255) NOT NULL,
  nombre VARCHAR(100) NOT NULL,
  role VARCHAR(10) DEFAULT 'alumno' CHECK(role IN ('alumno', 'profe')),
  seccion VARCHAR(10),
  created_at TIMESTAMP DEFAULT NOW()
);

CREATE TABLE niveles (
  id SERIAL PRIMARY KEY,
  numero INTEGER NOT NULL,
  titulo VARCHAR(200) NOT NULL,
  enunciado TEXT NOT NULL,
  pista TEXT,
  casos_prueba JSONB
);

CREATE TABLE pastes (
  id VARCHAR(8) PRIMARY KEY,
  user_id INTEGER NOT NULL REFERENCES users(id),
  nivel_id INTEGER NOT NULL REFERENCES niveles(id),
  content TEXT NOT NULL,
  estado VARCHAR(15) DEFAULT 'pendiente' CHECK(estado IN ('pendiente', 'aprobado', 'rechazado')),
  nota_profe TEXT,
  created_at TIMESTAMP DEFAULT NOW()
);

CREATE INDEX idx_pastes_nivel ON pastes(nivel_id);
CREATE INDEX idx_pastes_user ON pastes(user_id);
CREATE INDEX idx_users_seccion ON users(seccion);
