-- Run once on an existing FoodBridge database.
-- New installations should use database/schema.sql instead.
--
-- Existing accounts are marked verified so the new check does not lock them out.
-- Google accounts are marked verified automatically by the application.

ALTER TABLE accounts
  ADD COLUMN email_verified TINYINT(1) NOT NULL DEFAULT 0 AFTER password_hash,
  ADD COLUMN otp_hash CHAR(64) NULL AFTER email_verified,
  ADD COLUMN otp_expires_at DATETIME NULL AFTER otp_hash,
  ADD COLUMN otp_attempts SMALLINT UNSIGNED NOT NULL DEFAULT 0 AFTER otp_expires_at,
  ADD COLUMN otp_last_sent_at DATETIME NULL AFTER otp_attempts,
  ADD COLUMN otp_send_window_start DATETIME NULL AFTER otp_last_sent_at,
  ADD COLUMN otp_send_count SMALLINT UNSIGNED NOT NULL DEFAULT 0 AFTER otp_send_window_start;

UPDATE accounts SET email_verified = 1;
