-- Run once on an existing FoodBridge database.
-- New installations should use database/schema.sql instead.

ALTER TABLE accounts
  ADD COLUMN supabase_uid VARCHAR(64) NULL AFTER id,
  ADD UNIQUE KEY uq_accounts_supabase_uid (supabase_uid);

ALTER TABLE foods
  ADD COLUMN photo_url VARCHAR(500) NULL AFTER pickup_address,
  ADD COLUMN latitude DECIMAL(10, 7) NULL AFTER photo_url,
  ADD COLUMN longitude DECIMAL(10, 7) NULL AFTER latitude,
  ADD CONSTRAINT chk_foods_latitude
    CHECK (latitude IS NULL OR latitude BETWEEN -90 AND 90),
  ADD CONSTRAINT chk_foods_longitude
    CHECK (longitude IS NULL OR longitude BETWEEN -180 AND 180);
