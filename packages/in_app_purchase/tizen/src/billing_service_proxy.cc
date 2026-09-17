// Copyright 2025 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "billing_service_proxy.h"

BillingWrapper::BillingWrapper() = default;

BillingWrapper::~BillingWrapper() = default;

bool BillingWrapper::Initialize() {
  get_products_list = &ftpw_in_app_purchase_service_billing_get_products_list;
  get_purchase_list = &ftpw_in_app_purchase_service_billing_get_purchase_list;
  is_service_available = &ftpw_in_app_purchase_service_billing_is_service_available;
  buyitem = &ftpw_in_app_purchase_service_billing_buyitem;
  set_buyitem_cb = &ftpw_in_app_purchase_service_billing_set_buyitem_cb;
  verify_invoice = &ftpw_in_app_purchase_service_billing_verify_invoice;
  return true;
}

bool BillingWrapper::service_billing_get_products_list(
    const char *app_id, const char *country_code, int page_size,
    int page_number, const char *check_value,
    billing_payment_api_cb callback, void *user_data) {
  if (get_products_list) {
    return get_products_list(app_id, country_code, page_size, page_number,
                             check_value, callback, user_data);
  }
  return false;
}

bool BillingWrapper::service_billing_get_purchase_list(
    const char *app_id, const char *custom_id, const char *country_code,
    int page_number, const char *check_value,
    billing_payment_api_cb callback, void *user_data) {
  if (get_purchase_list) {
    return get_purchase_list(app_id, custom_id, country_code, page_number,
                             check_value, callback, user_data);
  }
  return false;
}

bool BillingWrapper::service_billing_buyitem(const char *app_id,
                                             const char *detail_info) {
  if (buyitem) {
    return buyitem(app_id, detail_info);
  }
  return false;
}

void BillingWrapper::service_billing_set_buyitem_cb(billing_buyitem_cb callback,
                                                    void *user_data) {
  if (set_buyitem_cb) {
    return set_buyitem_cb(callback, user_data);
  }
  return;
}

bool BillingWrapper::service_billing_is_service_available(
    billing_payment_api_cb callback, void *user_data) {
  if (is_service_available) {
    return is_service_available(callback, user_data);
  }
  return false;
}

bool BillingWrapper::service_billing_verify_invoice(
    const char *app_id, const char *custom_id, const char *invoice_id,
    const char *country_code,
    billing_payment_api_cb callback, void *user_data) {
  if (verify_invoice) {
    return verify_invoice(app_id, custom_id, invoice_id, country_code,
                          callback, user_data);
  }
  return false;
}
