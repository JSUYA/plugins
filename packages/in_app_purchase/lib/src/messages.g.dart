// Autogenerated from Pigeon (v22.7.2), do not edit directly.
// See also: https://pub.dev/packages/pigeon
// ignore_for_file: public_member_api_docs, non_constant_identifier_names, avoid_as, unused_import, unnecessary_parenthesis, prefer_null_aware_operators, omit_local_variable_types, unused_shown_name, unnecessary_import, no_leading_underscores_for_local_identifiers

import 'dart:async';
import 'dart:typed_data' show Float64List, Int32List, Int64List, Uint8List;

import 'package:flutter/foundation.dart' show ReadBuffer, WriteBuffer;
import 'package:flutter/services.dart';

PlatformException _createConnectionError(String channelName) {
  return PlatformException(
    code: 'channel-error',
    message: 'Unable to establish connection on channel: "$channelName".',
  );
}

/// Dart wrapper around [`ProductsListApiResult`] in (https://developer.samsung.com/smarttv/develop/api-references/samsung-product-api-references/billing-api.html).
///
/// Defines a dictionary for product list data returned by the getProductsList API.
/// This only can be used in [BillingManager.requestProducts].
class ProductsListApiResult {
  ProductsListApiResult({
    required this.cpStatus,
    this.cpResult,
    required this.totalCount,
    required this.checkValue,
    required this.itemDetails,
  });

  /// DPI result code.
  /// Returns "100000" on success and other codes on failure.
  String cpStatus;

  /// The result message.
  /// "EOF":Last page of the product list.
  /// "hasNext:TRUE" Product list has further pages.
  /// Other error message, depending on the DPI result code.
  String? cpResult;

  /// Total number of invoices.
  int totalCount;

  /// Security check value.
  String checkValue;

  /// ItemDetails in JSON format
  List<Map<Object?, Object?>?> itemDetails;

  Object encode() {
    return <Object?>[cpStatus, cpResult, totalCount, checkValue, itemDetails];
  }

  static ProductsListApiResult decode(Object result) {
    result as List<Object?>;
    return ProductsListApiResult(
      cpStatus: result[0]! as String,
      cpResult: result[1] as String?,
      totalCount: result[2]! as int,
      checkValue: result[3]! as String,
      itemDetails:
          (result[4] as List<Object?>?)!.cast<Map<Object?, Object?>?>(),
    );
  }
}

/// Dart wrapper around [`GetUserPurchaseListAPIResult`] in (https://developer.samsung.com/smarttv/develop/api-references/samsung-product-api-references/billing-api.html).
///
/// Defines a dictionary for data returned by the getUserPurchaseList API.
/// This only can be used in [BillingManager.requestPurchases]
class GetUserPurchaseListAPIResult {
  GetUserPurchaseListAPIResult({
    required this.cpStatus,
    this.cpResult,
    required this.totalCount,
    required this.checkValue,
    required this.invoiceDetails,
  });

  /// It returns "100000" in success and other codes in failure. Refer to DPI Error Code.
  String cpStatus;

  /// The result message:
  /// "EOF":Last page of the product list
  /// "hasNext:TRUE" Product list has further pages
  /// Other error message, depending on the DPI result code
  String? cpResult;

  /// Total number of invoices.
  int totalCount;

  /// Security check value.
  String checkValue;

  /// InvoiceDetailsin JSON format.
  List<Map<Object?, Object?>?> invoiceDetails;

  Object encode() {
    return <Object?>[
      cpStatus,
      cpResult,
      totalCount,
      checkValue,
      invoiceDetails,
    ];
  }

  static GetUserPurchaseListAPIResult decode(Object result) {
    result as List<Object?>;
    return GetUserPurchaseListAPIResult(
      cpStatus: result[0]! as String,
      cpResult: result[1] as String?,
      totalCount: result[2]! as int,
      checkValue: result[3]! as String,
      invoiceDetails:
          (result[4] as List<Object?>?)!.cast<Map<Object?, Object?>?>(),
    );
  }
}

/// Dart wrapper around [`BillingBuyData`](https://developer.samsung.com/smarttv/develop/api-references/samsung-product-api-references/billing-api.html#BillingBuyData).
///
/// Defines the payment result and information.
class BillingBuyData {
  BillingBuyData({required this.payResult, required this.payDetails});

  /// The payment result
  String payResult;

  /// The payment information. It is same with paymentDetails param of buyItem.
  Map<String, String> payDetails;

  Object encode() {
    return <Object?>[payResult, payDetails];
  }

  static BillingBuyData decode(Object result) {
    result as List<Object?>;
    return BillingBuyData(
      payResult: result[0]! as String,
      payDetails: (result[1] as Map<Object?, Object?>?)!.cast<String, String>(),
    );
  }
}

/// Dart wrapper around [`VerifyInvoiceAPIResult`] in (https://developer.samsung.com/smarttv/develop/api-references/samsung-product-api-references/billing-api.html).
///
/// This only can be used in [BillingManager.verifyInvoice].
class VerifyInvoiceAPIResult {
  VerifyInvoiceAPIResult({
    required this.cpStatus,
    this.cpResult,
    required this.appId,
    required this.invoiceId,
  });

  /// DPI result code. Returns "100000" on success and other codes on failure.
  String cpStatus;

  /// The result message:
  /// "SUCCESS" and Other error message, depending on the DPI result code.
  String? cpResult;

  /// The application ID.
  String appId;

  /// Invoice ID that you want to verify whether a purchase was successful.
  String invoiceId;

  Object encode() {
    return <Object?>[cpStatus, cpResult, appId, invoiceId];
  }

  static VerifyInvoiceAPIResult decode(Object result) {
    result as List<Object?>;
    return VerifyInvoiceAPIResult(
      cpStatus: result[0]! as String,
      cpResult: result[1] as String?,
      appId: result[2]! as String,
      invoiceId: result[3]! as String,
    );
  }
}

/// Dart wrapper around [`ServiceAvailableAPIResult`] in (https://developer.samsung.com/smarttv/develop/api-references/samsung-product-api-references/billing-api.html).
///
/// Defines a dictionary for data returned by the IsServiceAvailable API.
/// This only can be used in [BillingManager.isAvailable].
class ServiceAvailableAPIResult {
  ServiceAvailableAPIResult({
    required this.status,
    required this.result,
    required this.serviceYn,
  });

  /// The result code of connecting to billing server.
  /// Returns "100000" on success and other codes on failure.
  String status;

  /// The result message of connecting to billing server.
  /// Returns "Success" on success.
  String result;

  /// Returns "Y" if the service is available.
  /// It will be null, if disconnect to billing server.
  String serviceYn;

  Object encode() {
    return <Object?>[status, result, serviceYn];
  }

  static ServiceAvailableAPIResult decode(Object result) {
    result as List<Object?>;
    return ServiceAvailableAPIResult(
      status: result[0]! as String,
      result: result[1]! as String,
      serviceYn: result[2]! as String,
    );
  }
}

class ProductMessage {
  ProductMessage({
    required this.appId,
    required this.countryCode,
    required this.pageSize,
    required this.pageNum,
    required this.checkValue,
  });

  /// Application ID.
  String appId;

  /// TV country code.
  String countryCode;

  /// Number of products retrieved per page (maximum 100).
  int pageSize;

  /// Requested page number (1 ~ N).
  int pageNum;

  /// Security check value. Required parameters = "appId" + "countryCode".
  /// The check value is used by the DPI service to verify API requests.
  /// It is a Base64 hash generated by applying the HMAC SHA256 algorithm on a concatenated string of parameters using the DPI security key.
  String checkValue;

  Object encode() {
    return <Object?>[appId, countryCode, pageSize, pageNum, checkValue];
  }

  static ProductMessage decode(Object result) {
    result as List<Object?>;
    return ProductMessage(
      appId: result[0]! as String,
      countryCode: result[1]! as String,
      pageSize: result[2]! as int,
      pageNum: result[3]! as int,
      checkValue: result[4]! as String,
    );
  }
}

class PurchaseMessage {
  PurchaseMessage({
    required this.appId,
    required this.customId,
    required this.countryCode,
    required this.pageNum,
    required this.checkValue,
  });

  /// Application ID.
  String appId;

  /// Same value as "OrderCustomID" parameter for the BuyItem API (Samsung Account UID)
  String customId;

  /// TV country code.
  String countryCode;

  /// Requested page number (1 ~ N).
  int pageNum;

  /// Security check value. Required parameters = "appId" + "customId" + "countryCode" + "ItemType" + "pageNumber".
  /// ItemType, MUST use 2 as value ("all items")
  String checkValue;

  Object encode() {
    return <Object?>[appId, customId, countryCode, pageNum, checkValue];
  }

  static PurchaseMessage decode(Object result) {
    result as List<Object?>;
    return PurchaseMessage(
      appId: result[0]! as String,
      customId: result[1]! as String,
      countryCode: result[2]! as String,
      pageNum: result[3]! as int,
      checkValue: result[4]! as String,
    );
  }
}

class OrderDetails {
  OrderDetails({
    required this.orderItemId,
    required this.orderTitle,
    required this.orderTotal,
    required this.orderCurrencyId,
    required this.orderCustomId,
  });

  String orderItemId;

  String orderTitle;

  String orderTotal;

  String orderCurrencyId;

  String orderCustomId;

  Object encode() {
    return <Object?>[
      orderItemId,
      orderTitle,
      orderTotal,
      orderCurrencyId,
      orderCustomId,
    ];
  }

  static OrderDetails decode(Object result) {
    result as List<Object?>;
    return OrderDetails(
      orderItemId: result[0]! as String,
      orderTitle: result[1]! as String,
      orderTotal: result[2]! as String,
      orderCurrencyId: result[3]! as String,
      orderCustomId: result[4]! as String,
    );
  }
}

class BuyInfoMessage {
  BuyInfoMessage({required this.appId, required this.payDetials});

  /// Application ID.
  String appId;

  /// Payment parameters.
  OrderDetails payDetials;

  Object encode() {
    return <Object?>[appId, payDetials];
  }

  static BuyInfoMessage decode(Object result) {
    result as List<Object?>;
    return BuyInfoMessage(
      appId: result[0]! as String,
      payDetials: result[1]! as OrderDetails,
    );
  }
}

class InvoiceMessage {
  InvoiceMessage({
    required this.appId,
    required this.customId,
    required this.invoiceId,
    required this.countryCode,
  });

  /// Application ID.
  String appId;

  /// Same value as "OrderCustomID" parameter for the BuyItem API (Samsung Account UID).
  String customId;

  /// Invoice ID that you want to verify whether a purchase was successful.
  String invoiceId;

  ///  TV country code.
  String countryCode;

  Object encode() {
    return <Object?>[appId, customId, invoiceId, countryCode];
  }

  static InvoiceMessage decode(Object result) {
    result as List<Object?>;
    return InvoiceMessage(
      appId: result[0]! as String,
      customId: result[1]! as String,
      invoiceId: result[2]! as String,
      countryCode: result[3]! as String,
    );
  }
}

class _PigeonCodec extends StandardMessageCodec {
  const _PigeonCodec();
  @override
  void writeValue(WriteBuffer buffer, Object? value) {
    if (value is int) {
      buffer.putUint8(4);
      buffer.putInt64(value);
    } else if (value is ProductsListApiResult) {
      buffer.putUint8(129);
      writeValue(buffer, value.encode());
    } else if (value is GetUserPurchaseListAPIResult) {
      buffer.putUint8(130);
      writeValue(buffer, value.encode());
    } else if (value is BillingBuyData) {
      buffer.putUint8(131);
      writeValue(buffer, value.encode());
    } else if (value is VerifyInvoiceAPIResult) {
      buffer.putUint8(132);
      writeValue(buffer, value.encode());
    } else if (value is ServiceAvailableAPIResult) {
      buffer.putUint8(133);
      writeValue(buffer, value.encode());
    } else if (value is ProductMessage) {
      buffer.putUint8(134);
      writeValue(buffer, value.encode());
    } else if (value is PurchaseMessage) {
      buffer.putUint8(135);
      writeValue(buffer, value.encode());
    } else if (value is OrderDetails) {
      buffer.putUint8(136);
      writeValue(buffer, value.encode());
    } else if (value is BuyInfoMessage) {
      buffer.putUint8(137);
      writeValue(buffer, value.encode());
    } else if (value is InvoiceMessage) {
      buffer.putUint8(138);
      writeValue(buffer, value.encode());
    } else {
      super.writeValue(buffer, value);
    }
  }

  @override
  Object? readValueOfType(int type, ReadBuffer buffer) {
    switch (type) {
      case 129:
        return ProductsListApiResult.decode(readValue(buffer)!);
      case 130:
        return GetUserPurchaseListAPIResult.decode(readValue(buffer)!);
      case 131:
        return BillingBuyData.decode(readValue(buffer)!);
      case 132:
        return VerifyInvoiceAPIResult.decode(readValue(buffer)!);
      case 133:
        return ServiceAvailableAPIResult.decode(readValue(buffer)!);
      case 134:
        return ProductMessage.decode(readValue(buffer)!);
      case 135:
        return PurchaseMessage.decode(readValue(buffer)!);
      case 136:
        return OrderDetails.decode(readValue(buffer)!);
      case 137:
        return BuyInfoMessage.decode(readValue(buffer)!);
      case 138:
        return InvoiceMessage.decode(readValue(buffer)!);
      default:
        return super.readValueOfType(type, buffer);
    }
  }
}

class InAppPurchaseApi {
  /// Constructor for [InAppPurchaseApi].  The [binaryMessenger] named argument is
  /// available for dependency injection.  If it is left null, the default
  /// BinaryMessenger will be used which routes to the host platform.
  InAppPurchaseApi({
    BinaryMessenger? binaryMessenger,
    String messageChannelSuffix = '',
  }) : pigeonVar_binaryMessenger = binaryMessenger,
       pigeonVar_messageChannelSuffix =
           messageChannelSuffix.isNotEmpty ? '.$messageChannelSuffix' : '';
  final BinaryMessenger? pigeonVar_binaryMessenger;

  static const MessageCodec<Object?> pigeonChannelCodec = _PigeonCodec();

  final String pigeonVar_messageChannelSuffix;

  /// Retrieves the list of products registered on the Billing (DPI) server.
  Future<ProductsListApiResult> getProductsList(ProductMessage product) async {
    final String pigeonVar_channelName =
        'dev.flutter.pigeon.in_app_purchase_tizen.InAppPurchaseApi.getProductsList$pigeonVar_messageChannelSuffix';
    final BasicMessageChannel<Object?> pigeonVar_channel =
        BasicMessageChannel<Object?>(
          pigeonVar_channelName,
          pigeonChannelCodec,
          binaryMessenger: pigeonVar_binaryMessenger,
        );
    final List<Object?>? pigeonVar_replyList =
        await pigeonVar_channel.send(<Object?>[product]) as List<Object?>?;
    if (pigeonVar_replyList == null) {
      throw _createConnectionError(pigeonVar_channelName);
    } else if (pigeonVar_replyList.length > 1) {
      throw PlatformException(
        code: pigeonVar_replyList[0]! as String,
        message: pigeonVar_replyList[1] as String?,
        details: pigeonVar_replyList[2],
      );
    } else if (pigeonVar_replyList[0] == null) {
      throw PlatformException(
        code: 'null-error',
        message: 'Host platform returned null value for non-null return value.',
      );
    } else {
      return (pigeonVar_replyList[0] as ProductsListApiResult?)!;
    }
  }

  /// Retrieves the user's purchase list.
  Future<GetUserPurchaseListAPIResult> getUserPurchaseList(
    PurchaseMessage purchase,
  ) async {
    final String pigeonVar_channelName =
        'dev.flutter.pigeon.in_app_purchase_tizen.InAppPurchaseApi.getUserPurchaseList$pigeonVar_messageChannelSuffix';
    final BasicMessageChannel<Object?> pigeonVar_channel =
        BasicMessageChannel<Object?>(
          pigeonVar_channelName,
          pigeonChannelCodec,
          binaryMessenger: pigeonVar_binaryMessenger,
        );
    final List<Object?>? pigeonVar_replyList =
        await pigeonVar_channel.send(<Object?>[purchase]) as List<Object?>?;
    if (pigeonVar_replyList == null) {
      throw _createConnectionError(pigeonVar_channelName);
    } else if (pigeonVar_replyList.length > 1) {
      throw PlatformException(
        code: pigeonVar_replyList[0]! as String,
        message: pigeonVar_replyList[1] as String?,
        details: pigeonVar_replyList[2],
      );
    } else if (pigeonVar_replyList[0] == null) {
      throw PlatformException(
        code: 'null-error',
        message: 'Host platform returned null value for non-null return value.',
      );
    } else {
      return (pigeonVar_replyList[0] as GetUserPurchaseListAPIResult?)!;
    }
  }

  /// Enables implementing the Samsung Checkout Client module within the application.
  /// After authenticating the purchase information through the application, the user can proceed to purchase payment.
  Future<BillingBuyData> buyItem(BuyInfoMessage buyInfo) async {
    final String pigeonVar_channelName =
        'dev.flutter.pigeon.in_app_purchase_tizen.InAppPurchaseApi.buyItem$pigeonVar_messageChannelSuffix';
    final BasicMessageChannel<Object?> pigeonVar_channel =
        BasicMessageChannel<Object?>(
          pigeonVar_channelName,
          pigeonChannelCodec,
          binaryMessenger: pigeonVar_binaryMessenger,
        );
    final List<Object?>? pigeonVar_replyList =
        await pigeonVar_channel.send(<Object?>[buyInfo]) as List<Object?>?;
    if (pigeonVar_replyList == null) {
      throw _createConnectionError(pigeonVar_channelName);
    } else if (pigeonVar_replyList.length > 1) {
      throw PlatformException(
        code: pigeonVar_replyList[0]! as String,
        message: pigeonVar_replyList[1] as String?,
        details: pigeonVar_replyList[2],
      );
    } else if (pigeonVar_replyList[0] == null) {
      throw PlatformException(
        code: 'null-error',
        message: 'Host platform returned null value for non-null return value.',
      );
    } else {
      return (pigeonVar_replyList[0] as BillingBuyData?)!;
    }
  }

  /// Checks whether a purchase, corresponding to a specific "InvoiceID", was successful.
  Future<VerifyInvoiceAPIResult> verifyInvoice(InvoiceMessage invoice) async {
    final String pigeonVar_channelName =
        'dev.flutter.pigeon.in_app_purchase_tizen.InAppPurchaseApi.verifyInvoice$pigeonVar_messageChannelSuffix';
    final BasicMessageChannel<Object?> pigeonVar_channel =
        BasicMessageChannel<Object?>(
          pigeonVar_channelName,
          pigeonChannelCodec,
          binaryMessenger: pigeonVar_binaryMessenger,
        );
    final List<Object?>? pigeonVar_replyList =
        await pigeonVar_channel.send(<Object?>[invoice]) as List<Object?>?;
    if (pigeonVar_replyList == null) {
      throw _createConnectionError(pigeonVar_channelName);
    } else if (pigeonVar_replyList.length > 1) {
      throw PlatformException(
        code: pigeonVar_replyList[0]! as String,
        message: pigeonVar_replyList[1] as String?,
        details: pigeonVar_replyList[2],
      );
    } else if (pigeonVar_replyList[0] == null) {
      throw PlatformException(
        code: 'null-error',
        message: 'Host platform returned null value for non-null return value.',
      );
    } else {
      return (pigeonVar_replyList[0] as VerifyInvoiceAPIResult?)!;
    }
  }

  /// Checks whether the Billing server is available.
  Future<bool> isServiceAvailable() async {
    final String pigeonVar_channelName =
        'dev.flutter.pigeon.in_app_purchase_tizen.InAppPurchaseApi.isServiceAvailable$pigeonVar_messageChannelSuffix';
    final BasicMessageChannel<Object?> pigeonVar_channel =
        BasicMessageChannel<Object?>(
          pigeonVar_channelName,
          pigeonChannelCodec,
          binaryMessenger: pigeonVar_binaryMessenger,
        );
    final List<Object?>? pigeonVar_replyList =
        await pigeonVar_channel.send(null) as List<Object?>?;
    if (pigeonVar_replyList == null) {
      throw _createConnectionError(pigeonVar_channelName);
    } else if (pigeonVar_replyList.length > 1) {
      throw PlatformException(
        code: pigeonVar_replyList[0]! as String,
        message: pigeonVar_replyList[1] as String?,
        details: pigeonVar_replyList[2],
      );
    } else if (pigeonVar_replyList[0] == null) {
      throw PlatformException(
        code: 'null-error',
        message: 'Host platform returned null value for non-null return value.',
      );
    } else {
      return (pigeonVar_replyList[0] as bool?)!;
    }
  }

  Future<String> getCustomId() async {
    final String pigeonVar_channelName =
        'dev.flutter.pigeon.in_app_purchase_tizen.InAppPurchaseApi.getCustomId$pigeonVar_messageChannelSuffix';
    final BasicMessageChannel<Object?> pigeonVar_channel =
        BasicMessageChannel<Object?>(
          pigeonVar_channelName,
          pigeonChannelCodec,
          binaryMessenger: pigeonVar_binaryMessenger,
        );
    final List<Object?>? pigeonVar_replyList =
        await pigeonVar_channel.send(null) as List<Object?>?;
    if (pigeonVar_replyList == null) {
      throw _createConnectionError(pigeonVar_channelName);
    } else if (pigeonVar_replyList.length > 1) {
      throw PlatformException(
        code: pigeonVar_replyList[0]! as String,
        message: pigeonVar_replyList[1] as String?,
        details: pigeonVar_replyList[2],
      );
    } else if (pigeonVar_replyList[0] == null) {
      throw PlatformException(
        code: 'null-error',
        message: 'Host platform returned null value for non-null return value.',
      );
    } else {
      return (pigeonVar_replyList[0] as String?)!;
    }
  }

  Future<String> getCountryCode() async {
    final String pigeonVar_channelName =
        'dev.flutter.pigeon.in_app_purchase_tizen.InAppPurchaseApi.getCountryCode$pigeonVar_messageChannelSuffix';
    final BasicMessageChannel<Object?> pigeonVar_channel =
        BasicMessageChannel<Object?>(
          pigeonVar_channelName,
          pigeonChannelCodec,
          binaryMessenger: pigeonVar_binaryMessenger,
        );
    final List<Object?>? pigeonVar_replyList =
        await pigeonVar_channel.send(null) as List<Object?>?;
    if (pigeonVar_replyList == null) {
      throw _createConnectionError(pigeonVar_channelName);
    } else if (pigeonVar_replyList.length > 1) {
      throw PlatformException(
        code: pigeonVar_replyList[0]! as String,
        message: pigeonVar_replyList[1] as String?,
        details: pigeonVar_replyList[2],
      );
    } else if (pigeonVar_replyList[0] == null) {
      throw PlatformException(
        code: 'null-error',
        message: 'Host platform returned null value for non-null return value.',
      );
    } else {
      return (pigeonVar_replyList[0] as String?)!;
    }
  }
}
