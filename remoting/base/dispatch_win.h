// This file was GENERATED by command:
//     pump.py dispatch_win.h.pump
// DO NOT EDIT BY HAND!!!

// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_BASE_IDISPATCH_DRIVER_WIN_H_
#define REMOTING_BASE_IDISPATCH_DRIVER_WIN_H_

#include <oaidl.h>

#include "base/basictypes.h"
#include "base/template_util.h"
#include "base/win/scoped_variant.h"

namespace remoting {

namespace dispatch {

namespace internal {

// A helper wrapper for |VARIANTARG| that is used to pass parameters to and from
// IDispatch::Invoke(). The latter accepts parameters as an array of
// |VARIANTARG| structures. The calling convention of IDispatch::Invoke() is:
//   - [in] parameters are initialized and freed if needed by the caller.
//   - [out] parameters are initialized by IDispatch::Invoke(). It is up to
//         the caller to free leakable variants (such as VT_DISPATCH).
//   - [in] [out] parameters are combination of both: the caller initializes
//         them before the call and the callee assigns new values correctly
//         freeing leakable variants.
//
// Using |ScopedVariantArg| instead of naked |VARIANTARG| ensures that
// the resources allocated during the call will be properly freed. It also
// provides wrapping methods that convert between C++ types and VARIANTs.
// At the moment the only supported parameter type is |VARIANT| (or
// |VARIANTARG|).
//
// It must be possible to cast a pointer to an array of |ScopedVariantArg| to
// a pointer to an array of |VARIANTARG| structures.
class ScopedVariantArg : public VARIANTARG {
 public:
  ScopedVariantArg() {
    vt = VT_EMPTY;
  }

  ~ScopedVariantArg() {
    VariantClear(this);
  }

  // Wrap() routines pack the input parameters into VARIANTARG structures so
  // that they can be passed to IDispatch::Invoke.

  HRESULT Wrap(const VARIANT& param) {
    DCHECK(vt == VT_EMPTY);
    return VariantCopy(this, &param);
  }

  HRESULT Wrap(VARIANT* const & param) {
    DCHECK(vt == VT_EMPTY);

    // Make the input value of an [in] [out] parameter visible to
    // IDispatch::Invoke().
    //
    // N.B. We treat both [out] and [in] [out] parameters as [in] [out]. In
    // other words the caller is always responsible for initializing and freeing
    // [out] and [in] [out] parameters.
    Swap(param);
    return S_OK;
  }

  // Unwrap() routines unpack the output parameters from VARIANTARG structures
  // to the locations specified by the caller.

  void Unwrap(const VARIANT& param_out) {
    // Do nothing for an [in] parameter.
  }

  void Unwrap(VARIANT* const & param_out) {
    // Return the output value of an [in] [out] parameter to the caller.
    Swap(param_out);
  }

 private:
  // Exchanges the value (and ownership) of the passed VARIANT with the one
  // wrapped by |ScopedVariantArg|.
  void Swap(VARIANT* other) {
    VARIANT temp = *other;
    *other = *this;
    *static_cast<VARIANTARG*>(this) = temp;
  }

  DISALLOW_COPY_AND_ASSIGN(ScopedVariantArg);
};

// Make sure the layouts of |VARIANTARG| and |ScopedVariantArg| are identical.
static_assert(sizeof(ScopedVariantArg) == sizeof(VARIANTARG),
              "scoped variant arg should not add data members");

}  // namespace internal

// Invoke() is a convenience wrapper for IDispatch::Invoke. It takes care of
// calling the desired method by its ID and implements logic for passing
// a variable number of in/out parameters to the called method.
//
// The calling convention is:
//   - [in] parameters are passsed as a constant reference or by value.
//   - [out] and [in] [out] parameters are passed by pointer. The pointed value
//         is overwritten when the function returns. The pointed-to value must
//         be initialized before the call, and will be replaced when it returns.
//         [out] parameters may be initialized to VT_EMPTY.
//
// Current limitations:
//   - more than 7 parameters are not supported.
//   - the method ID cannot be cached and reused.
//   - VARIANT is the only supported parameter type at the moment.

HRESULT Invoke(IDispatch* object,
               LPCOLESTR const_name,
               WORD flags,
               VARIANT* const & result_out) {
  // Retrieve the ID of the method to be called.
  DISPID disp_id;
  LPOLESTR name = const_cast<LPOLESTR>(const_name);
  HRESULT hr = object->GetIDsOfNames(
      IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &disp_id);
  if (FAILED(hr))
    return hr;

  // Request the return value if asked by the caller.
  internal::ScopedVariantArg result;
  VARIANT* disp_result = NULL;
  if (result_out != NULL)
    disp_result = &result;


  // Invoke the method passing the parameters via the DISPPARAMS structure.
  // DISPATCH_PROPERTYPUT and DISPATCH_PROPERTYPUTREF require the parameter of
  // the property setter to be named, so |cNamedArgs| and |rgdispidNamedArgs|
  // structure members should be initialized.
  DISPPARAMS disp_params = { NULL, NULL, 0, 0 };
  DISPID dispid_named = DISPID_PROPERTYPUT;
  if (flags == DISPATCH_PROPERTYPUT || flags == DISPATCH_PROPERTYPUTREF) {
    disp_params.cNamedArgs = 1;
    disp_params.rgdispidNamedArgs = &dispid_named;
  }

  hr = object->Invoke(disp_id, IID_NULL, LOCALE_USER_DEFAULT, flags,
                      &disp_params, disp_result, NULL, NULL);
  if (FAILED(hr))
    return hr;


  // Unwrap the return value.
  if (result_out != NULL) {
    result.Unwrap(result_out);
  }

  return S_OK;
}

template <typename P1>
HRESULT Invoke(IDispatch* object,
               LPCOLESTR const_name,
               WORD flags,
               const P1& p1,
               VARIANT* const & result_out) {
  // Retrieve the ID of the method to be called.
  DISPID disp_id;
  LPOLESTR name = const_cast<LPOLESTR>(const_name);
  HRESULT hr = object->GetIDsOfNames(
      IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &disp_id);
  if (FAILED(hr))
    return hr;

  // Request the return value if asked by the caller.
  internal::ScopedVariantArg result;
  VARIANT* disp_result = NULL;
  if (result_out != NULL)
    disp_result = &result;

  // Wrap the parameters into an array of VARIANT structures.
  internal::ScopedVariantArg disp_args[1];
  hr = disp_args[1 - 1].Wrap(p1);
  if (FAILED(hr))
    return hr;

  // Invoke the method passing the parameters via the DISPPARAMS structure.
  // DISPATCH_PROPERTYPUT and DISPATCH_PROPERTYPUTREF require the parameter of
  // the property setter to be named, so |cNamedArgs| and |rgdispidNamedArgs|
  // structure members should be initialized.
  DISPPARAMS disp_params = { disp_args, NULL, 1, 0 };
  DISPID dispid_named = DISPID_PROPERTYPUT;
  if (flags == DISPATCH_PROPERTYPUT || flags == DISPATCH_PROPERTYPUTREF) {
    disp_params.cNamedArgs = 1;
    disp_params.rgdispidNamedArgs = &dispid_named;
  }

  hr = object->Invoke(disp_id, IID_NULL, LOCALE_USER_DEFAULT, flags,
                      &disp_params, disp_result, NULL, NULL);
  if (FAILED(hr))
    return hr;

  // Unwrap the parameters.
  disp_args[1 - 1].Unwrap(p1);

  // Unwrap the return value.
  if (result_out != NULL) {
    result.Unwrap(result_out);
  }

  return S_OK;
}

template <typename P1, typename P2>
HRESULT Invoke(IDispatch* object,
               LPCOLESTR const_name,
               WORD flags,
               const P1& p1,
               const P2& p2,
               VARIANT* const & result_out) {
  // Retrieve the ID of the method to be called.
  DISPID disp_id;
  LPOLESTR name = const_cast<LPOLESTR>(const_name);
  HRESULT hr = object->GetIDsOfNames(
      IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &disp_id);
  if (FAILED(hr))
    return hr;

  // Request the return value if asked by the caller.
  internal::ScopedVariantArg result;
  VARIANT* disp_result = NULL;
  if (result_out != NULL)
    disp_result = &result;

  // Wrap the parameters into an array of VARIANT structures.
  internal::ScopedVariantArg disp_args[2];
  hr = disp_args[2 - 1].Wrap(p1);
  if (FAILED(hr))
    return hr;
  hr = disp_args[2 - 2].Wrap(p2);
  if (FAILED(hr))
    return hr;

  // Invoke the method passing the parameters via the DISPPARAMS structure.
  // DISPATCH_PROPERTYPUT and DISPATCH_PROPERTYPUTREF require the parameter of
  // the property setter to be named, so |cNamedArgs| and |rgdispidNamedArgs|
  // structure members should be initialized.
  DISPPARAMS disp_params = { disp_args, NULL, 2, 0 };
  DISPID dispid_named = DISPID_PROPERTYPUT;
  if (flags == DISPATCH_PROPERTYPUT || flags == DISPATCH_PROPERTYPUTREF) {
    disp_params.cNamedArgs = 1;
    disp_params.rgdispidNamedArgs = &dispid_named;
  }

  hr = object->Invoke(disp_id, IID_NULL, LOCALE_USER_DEFAULT, flags,
                      &disp_params, disp_result, NULL, NULL);
  if (FAILED(hr))
    return hr;

  // Unwrap the parameters.
  disp_args[2 - 1].Unwrap(p1);
  disp_args[2 - 2].Unwrap(p2);

  // Unwrap the return value.
  if (result_out != NULL) {
    result.Unwrap(result_out);
  }

  return S_OK;
}

template <typename P1, typename P2, typename P3>
HRESULT Invoke(IDispatch* object,
               LPCOLESTR const_name,
               WORD flags,
               const P1& p1,
               const P2& p2,
               const P3& p3,
               VARIANT* const & result_out) {
  // Retrieve the ID of the method to be called.
  DISPID disp_id;
  LPOLESTR name = const_cast<LPOLESTR>(const_name);
  HRESULT hr = object->GetIDsOfNames(
      IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &disp_id);
  if (FAILED(hr))
    return hr;

  // Request the return value if asked by the caller.
  internal::ScopedVariantArg result;
  VARIANT* disp_result = NULL;
  if (result_out != NULL)
    disp_result = &result;

  // Wrap the parameters into an array of VARIANT structures.
  internal::ScopedVariantArg disp_args[3];
  hr = disp_args[3 - 1].Wrap(p1);
  if (FAILED(hr))
    return hr;
  hr = disp_args[3 - 2].Wrap(p2);
  if (FAILED(hr))
    return hr;
  hr = disp_args[3 - 3].Wrap(p3);
  if (FAILED(hr))
    return hr;

  // Invoke the method passing the parameters via the DISPPARAMS structure.
  // DISPATCH_PROPERTYPUT and DISPATCH_PROPERTYPUTREF require the parameter of
  // the property setter to be named, so |cNamedArgs| and |rgdispidNamedArgs|
  // structure members should be initialized.
  DISPPARAMS disp_params = { disp_args, NULL, 3, 0 };
  DISPID dispid_named = DISPID_PROPERTYPUT;
  if (flags == DISPATCH_PROPERTYPUT || flags == DISPATCH_PROPERTYPUTREF) {
    disp_params.cNamedArgs = 1;
    disp_params.rgdispidNamedArgs = &dispid_named;
  }

  hr = object->Invoke(disp_id, IID_NULL, LOCALE_USER_DEFAULT, flags,
                      &disp_params, disp_result, NULL, NULL);
  if (FAILED(hr))
    return hr;

  // Unwrap the parameters.
  disp_args[3 - 1].Unwrap(p1);
  disp_args[3 - 2].Unwrap(p2);
  disp_args[3 - 3].Unwrap(p3);

  // Unwrap the return value.
  if (result_out != NULL) {
    result.Unwrap(result_out);
  }

  return S_OK;
}

template <typename P1, typename P2, typename P3, typename P4>
HRESULT Invoke(IDispatch* object,
               LPCOLESTR const_name,
               WORD flags,
               const P1& p1,
               const P2& p2,
               const P3& p3,
               const P4& p4,
               VARIANT* const & result_out) {
  // Retrieve the ID of the method to be called.
  DISPID disp_id;
  LPOLESTR name = const_cast<LPOLESTR>(const_name);
  HRESULT hr = object->GetIDsOfNames(
      IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &disp_id);
  if (FAILED(hr))
    return hr;

  // Request the return value if asked by the caller.
  internal::ScopedVariantArg result;
  VARIANT* disp_result = NULL;
  if (result_out != NULL)
    disp_result = &result;

  // Wrap the parameters into an array of VARIANT structures.
  internal::ScopedVariantArg disp_args[4];
  hr = disp_args[4 - 1].Wrap(p1);
  if (FAILED(hr))
    return hr;
  hr = disp_args[4 - 2].Wrap(p2);
  if (FAILED(hr))
    return hr;
  hr = disp_args[4 - 3].Wrap(p3);
  if (FAILED(hr))
    return hr;
  hr = disp_args[4 - 4].Wrap(p4);
  if (FAILED(hr))
    return hr;

  // Invoke the method passing the parameters via the DISPPARAMS structure.
  // DISPATCH_PROPERTYPUT and DISPATCH_PROPERTYPUTREF require the parameter of
  // the property setter to be named, so |cNamedArgs| and |rgdispidNamedArgs|
  // structure members should be initialized.
  DISPPARAMS disp_params = { disp_args, NULL, 4, 0 };
  DISPID dispid_named = DISPID_PROPERTYPUT;
  if (flags == DISPATCH_PROPERTYPUT || flags == DISPATCH_PROPERTYPUTREF) {
    disp_params.cNamedArgs = 1;
    disp_params.rgdispidNamedArgs = &dispid_named;
  }

  hr = object->Invoke(disp_id, IID_NULL, LOCALE_USER_DEFAULT, flags,
                      &disp_params, disp_result, NULL, NULL);
  if (FAILED(hr))
    return hr;

  // Unwrap the parameters.
  disp_args[4 - 1].Unwrap(p1);
  disp_args[4 - 2].Unwrap(p2);
  disp_args[4 - 3].Unwrap(p3);
  disp_args[4 - 4].Unwrap(p4);

  // Unwrap the return value.
  if (result_out != NULL) {
    result.Unwrap(result_out);
  }

  return S_OK;
}

template <typename P1, typename P2, typename P3, typename P4, typename P5>
HRESULT Invoke(IDispatch* object,
               LPCOLESTR const_name,
               WORD flags,
               const P1& p1,
               const P2& p2,
               const P3& p3,
               const P4& p4,
               const P5& p5,
               VARIANT* const & result_out) {
  // Retrieve the ID of the method to be called.
  DISPID disp_id;
  LPOLESTR name = const_cast<LPOLESTR>(const_name);
  HRESULT hr = object->GetIDsOfNames(
      IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &disp_id);
  if (FAILED(hr))
    return hr;

  // Request the return value if asked by the caller.
  internal::ScopedVariantArg result;
  VARIANT* disp_result = NULL;
  if (result_out != NULL)
    disp_result = &result;

  // Wrap the parameters into an array of VARIANT structures.
  internal::ScopedVariantArg disp_args[5];
  hr = disp_args[5 - 1].Wrap(p1);
  if (FAILED(hr))
    return hr;
  hr = disp_args[5 - 2].Wrap(p2);
  if (FAILED(hr))
    return hr;
  hr = disp_args[5 - 3].Wrap(p3);
  if (FAILED(hr))
    return hr;
  hr = disp_args[5 - 4].Wrap(p4);
  if (FAILED(hr))
    return hr;
  hr = disp_args[5 - 5].Wrap(p5);
  if (FAILED(hr))
    return hr;

  // Invoke the method passing the parameters via the DISPPARAMS structure.
  // DISPATCH_PROPERTYPUT and DISPATCH_PROPERTYPUTREF require the parameter of
  // the property setter to be named, so |cNamedArgs| and |rgdispidNamedArgs|
  // structure members should be initialized.
  DISPPARAMS disp_params = { disp_args, NULL, 5, 0 };
  DISPID dispid_named = DISPID_PROPERTYPUT;
  if (flags == DISPATCH_PROPERTYPUT || flags == DISPATCH_PROPERTYPUTREF) {
    disp_params.cNamedArgs = 1;
    disp_params.rgdispidNamedArgs = &dispid_named;
  }

  hr = object->Invoke(disp_id, IID_NULL, LOCALE_USER_DEFAULT, flags,
                      &disp_params, disp_result, NULL, NULL);
  if (FAILED(hr))
    return hr;

  // Unwrap the parameters.
  disp_args[5 - 1].Unwrap(p1);
  disp_args[5 - 2].Unwrap(p2);
  disp_args[5 - 3].Unwrap(p3);
  disp_args[5 - 4].Unwrap(p4);
  disp_args[5 - 5].Unwrap(p5);

  // Unwrap the return value.
  if (result_out != NULL) {
    result.Unwrap(result_out);
  }

  return S_OK;
}

template <typename P1, typename P2, typename P3, typename P4, typename P5,
    typename P6>
HRESULT Invoke(IDispatch* object,
               LPCOLESTR const_name,
               WORD flags,
               const P1& p1,
               const P2& p2,
               const P3& p3,
               const P4& p4,
               const P5& p5,
               const P6& p6,
               VARIANT* const & result_out) {
  // Retrieve the ID of the method to be called.
  DISPID disp_id;
  LPOLESTR name = const_cast<LPOLESTR>(const_name);
  HRESULT hr = object->GetIDsOfNames(
      IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &disp_id);
  if (FAILED(hr))
    return hr;

  // Request the return value if asked by the caller.
  internal::ScopedVariantArg result;
  VARIANT* disp_result = NULL;
  if (result_out != NULL)
    disp_result = &result;

  // Wrap the parameters into an array of VARIANT structures.
  internal::ScopedVariantArg disp_args[6];
  hr = disp_args[6 - 1].Wrap(p1);
  if (FAILED(hr))
    return hr;
  hr = disp_args[6 - 2].Wrap(p2);
  if (FAILED(hr))
    return hr;
  hr = disp_args[6 - 3].Wrap(p3);
  if (FAILED(hr))
    return hr;
  hr = disp_args[6 - 4].Wrap(p4);
  if (FAILED(hr))
    return hr;
  hr = disp_args[6 - 5].Wrap(p5);
  if (FAILED(hr))
    return hr;
  hr = disp_args[6 - 6].Wrap(p6);
  if (FAILED(hr))
    return hr;

  // Invoke the method passing the parameters via the DISPPARAMS structure.
  // DISPATCH_PROPERTYPUT and DISPATCH_PROPERTYPUTREF require the parameter of
  // the property setter to be named, so |cNamedArgs| and |rgdispidNamedArgs|
  // structure members should be initialized.
  DISPPARAMS disp_params = { disp_args, NULL, 6, 0 };
  DISPID dispid_named = DISPID_PROPERTYPUT;
  if (flags == DISPATCH_PROPERTYPUT || flags == DISPATCH_PROPERTYPUTREF) {
    disp_params.cNamedArgs = 1;
    disp_params.rgdispidNamedArgs = &dispid_named;
  }

  hr = object->Invoke(disp_id, IID_NULL, LOCALE_USER_DEFAULT, flags,
                      &disp_params, disp_result, NULL, NULL);
  if (FAILED(hr))
    return hr;

  // Unwrap the parameters.
  disp_args[6 - 1].Unwrap(p1);
  disp_args[6 - 2].Unwrap(p2);
  disp_args[6 - 3].Unwrap(p3);
  disp_args[6 - 4].Unwrap(p4);
  disp_args[6 - 5].Unwrap(p5);
  disp_args[6 - 6].Unwrap(p6);

  // Unwrap the return value.
  if (result_out != NULL) {
    result.Unwrap(result_out);
  }

  return S_OK;
}

template <typename P1, typename P2, typename P3, typename P4, typename P5,
    typename P6, typename P7>
HRESULT Invoke(IDispatch* object,
               LPCOLESTR const_name,
               WORD flags,
               const P1& p1,
               const P2& p2,
               const P3& p3,
               const P4& p4,
               const P5& p5,
               const P6& p6,
               const P7& p7,
               VARIANT* const & result_out) {
  // Retrieve the ID of the method to be called.
  DISPID disp_id;
  LPOLESTR name = const_cast<LPOLESTR>(const_name);
  HRESULT hr = object->GetIDsOfNames(
      IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &disp_id);
  if (FAILED(hr))
    return hr;

  // Request the return value if asked by the caller.
  internal::ScopedVariantArg result;
  VARIANT* disp_result = NULL;
  if (result_out != NULL)
    disp_result = &result;

  // Wrap the parameters into an array of VARIANT structures.
  internal::ScopedVariantArg disp_args[7];
  hr = disp_args[7 - 1].Wrap(p1);
  if (FAILED(hr))
    return hr;
  hr = disp_args[7 - 2].Wrap(p2);
  if (FAILED(hr))
    return hr;
  hr = disp_args[7 - 3].Wrap(p3);
  if (FAILED(hr))
    return hr;
  hr = disp_args[7 - 4].Wrap(p4);
  if (FAILED(hr))
    return hr;
  hr = disp_args[7 - 5].Wrap(p5);
  if (FAILED(hr))
    return hr;
  hr = disp_args[7 - 6].Wrap(p6);
  if (FAILED(hr))
    return hr;
  hr = disp_args[7 - 7].Wrap(p7);
  if (FAILED(hr))
    return hr;

  // Invoke the method passing the parameters via the DISPPARAMS structure.
  // DISPATCH_PROPERTYPUT and DISPATCH_PROPERTYPUTREF require the parameter of
  // the property setter to be named, so |cNamedArgs| and |rgdispidNamedArgs|
  // structure members should be initialized.
  DISPPARAMS disp_params = { disp_args, NULL, 7, 0 };
  DISPID dispid_named = DISPID_PROPERTYPUT;
  if (flags == DISPATCH_PROPERTYPUT || flags == DISPATCH_PROPERTYPUTREF) {
    disp_params.cNamedArgs = 1;
    disp_params.rgdispidNamedArgs = &dispid_named;
  }

  hr = object->Invoke(disp_id, IID_NULL, LOCALE_USER_DEFAULT, flags,
                      &disp_params, disp_result, NULL, NULL);
  if (FAILED(hr))
    return hr;

  // Unwrap the parameters.
  disp_args[7 - 1].Unwrap(p1);
  disp_args[7 - 2].Unwrap(p2);
  disp_args[7 - 3].Unwrap(p3);
  disp_args[7 - 4].Unwrap(p4);
  disp_args[7 - 5].Unwrap(p5);
  disp_args[7 - 6].Unwrap(p6);
  disp_args[7 - 7].Unwrap(p7);

  // Unwrap the return value.
  if (result_out != NULL) {
    result.Unwrap(result_out);
  }

  return S_OK;
}

} // namespace dispatch

} // namespace remoting

#endif // REMOTING_BASE_IDISPATCH_DRIVER_WIN_H_
