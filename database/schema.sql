CREATE TABLE IF NOT EXISTS accounts (
  id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  supabase_uid VARCHAR(64) DEFAULT NULL,
  name VARCHAR(160) NOT NULL,
  email VARCHAR(254) NOT NULL,
  password_hash VARCHAR(255) NOT NULL,
  email_verified TINYINT(1) NOT NULL DEFAULT 0,
  otp_hash CHAR(64) DEFAULT NULL,
  otp_expires_at DATETIME DEFAULT NULL,
  otp_attempts SMALLINT UNSIGNED NOT NULL DEFAULT 0,
  otp_last_sent_at DATETIME DEFAULT NULL,
  otp_send_window_start DATETIME DEFAULT NULL,
  otp_send_count SMALLINT UNSIGNED NOT NULL DEFAULT 0,
  role VARCHAR(16) NOT NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (id),
  UNIQUE KEY uq_accounts_email (email),
  UNIQUE KEY uq_accounts_supabase_uid (supabase_uid),
  CONSTRAINT chk_accounts_role CHECK (role IN ('hotel', 'ngo', 'admin'))
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS foods (
  id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  hotel_id BIGINT UNSIGNED NOT NULL,
  name VARCHAR(180) NOT NULL,
  category VARCHAR(80) NOT NULL,
  description TEXT NOT NULL,
  total_quantity DECIMAL(10, 2) NOT NULL,
  available_quantity DECIMAL(10, 2) NOT NULL,
  unit VARCHAR(32) NOT NULL,
  pickup_address VARCHAR(500) NOT NULL,
  photo_url VARCHAR(500) DEFAULT NULL,
  latitude DECIMAL(10, 7) DEFAULT NULL,
  longitude DECIMAL(10, 7) DEFAULT NULL,
  best_before DATE NOT NULL,
  status VARCHAR(16) NOT NULL DEFAULT 'available',
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (id),
  KEY ix_foods_hotel_created (hotel_id, created_at),
  KEY ix_foods_browse (status, best_before),
  CONSTRAINT fk_foods_hotel FOREIGN KEY (hotel_id) REFERENCES accounts (id),
  CONSTRAINT chk_foods_quantity CHECK (
    total_quantity > 0 AND available_quantity >= 0
    AND available_quantity <= total_quantity
  ),
  CONSTRAINT chk_foods_latitude CHECK (latitude IS NULL OR latitude BETWEEN -90 AND 90),
  CONSTRAINT chk_foods_longitude CHECK (longitude IS NULL OR longitude BETWEEN -180 AND 180),
  CONSTRAINT chk_foods_status CHECK (status IN ('available', 'fulfilled', 'closed'))
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS orders (
  id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  food_id BIGINT UNSIGNED NOT NULL,
  hotel_id BIGINT UNSIGNED NOT NULL,
  ngo_id BIGINT UNSIGNED NOT NULL,
  quantity DECIMAL(10, 2) NOT NULL,
  note VARCHAR(1000) NOT NULL DEFAULT '',
  status VARCHAR(16) NOT NULL DEFAULT 'pending',
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (id),
  KEY ix_orders_hotel_status (hotel_id, status, created_at),
  KEY ix_orders_ngo_status (ngo_id, status, created_at),
  KEY ix_orders_food_status (food_id, status),
  CONSTRAINT fk_orders_food FOREIGN KEY (food_id) REFERENCES foods (id),
  CONSTRAINT fk_orders_hotel FOREIGN KEY (hotel_id) REFERENCES accounts (id),
  CONSTRAINT fk_orders_ngo FOREIGN KEY (ngo_id) REFERENCES accounts (id),
  CONSTRAINT chk_orders_quantity CHECK (quantity > 0),
  CONSTRAINT chk_orders_status CHECK (status IN ('pending', 'accepted', 'rejected'))
) ENGINE=InnoDB;
