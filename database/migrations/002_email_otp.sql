-- Run once on an existing FoodBridge database.
-- New installations should use database/schema.sql instead.
--
-- Existing accounts are marked verified so the new check does not lock them out.
-- Google accounts are marked verified automatically by the application.
--
-- Each column is added in its own ALTER statement. This also works on TiDB,
-- which cannot reference a column added earlier in the same ALTER statement.

ALTER TABLE accounts
  ADD COLUMN email_verified TINYINT(1) NOT NULL DEFAULT 0 AFTER password_hash;

ALTER TABLE accounts
  ADD COLUMN otp_hash CHAR(64) NULL AFTER email_verified;

ALTER TABLE accounts
  ADD COLUMN otp_expires_at DATETIME NULL AFTER otp_hash;

ALTER TABLE accounts
  ADD COLUMN otp_attempts SMALLINT UNSIGNED NOT NULL DEFAULT 0 AFTER otp_expires_at;

ALTER TABLE accounts
  ADD COLUMN otp_last_sent_at DATETIME NULL AFTER otp_attempts;

ALTER TABLE accounts
  ADD COLUMN otp_send_window_start DATETIME NULL AFTER otp_last_sent_at;

ALTER TABLE accounts
  ADD COLUMN otp_send_count SMALLINT UNSIGNED NOT NULL DEFAULT 0 AFTER otp_send_window_start;

UPDATE accounts SET email_verified = 1;
