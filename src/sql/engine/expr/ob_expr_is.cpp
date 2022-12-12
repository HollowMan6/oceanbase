/**
 * Copyright (c) 2021 OceanBase
 * OceanBase CE is licensed under Mulan PubL v2.
 * You can use this software according to the terms and conditions of the Mulan PubL v2.
 * You may obtain a copy of Mulan PubL v2 at:
 *          http://license.coscl.org.cn/MulanPubL-2.0
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PubL v2 for more details.
 */

#define USING_LOG_PREFIX SQL_ENG
#include <math.h>
#include "sql/engine/expr/ob_expr_is.h"
#include "share/object/ob_obj_cast.h"
//#include "sql/engine/expr/ob_expr_promotion_util.h"
#include "objit/common/ob_item_type.h"
#include "sql/session/ob_sql_session_info.h"
#include "share/config/ob_server_config.h"

namespace oceanbase
{
using namespace common;
namespace sql
{

ObExprIsBase::ObExprIsBase(ObIAllocator &alloc,
                           ObExprOperatorType type,
                           const char *name)
    : ObRelationalExprOperator(alloc, type, name, 3)
{};

int ObExprIsBase::calc_result_type3(ObExprResType &type,
                                    ObExprResType &type1,
                                    ObExprResType &type2,
                                    ObExprResType &type3,
                                    ObExprTypeCtx &type_ctx) const
{
  int ret = OB_SUCCESS;
  ObRawExpr *raw_expr = get_raw_expr();
  ObOpRawExpr *op_expr = static_cast<ObOpRawExpr *>(raw_expr);
  if (OB_ISNULL(op_expr) || OB_UNLIKELY(op_expr->get_param_count() != 3)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("op raw expr is null", K(ret), K(op_expr));
  } else if (lib::is_oracle_mode() && type1.is_ext()) {
    type.set_int32();
    type.set_precision(DEFAULT_PRECISION_FOR_BOOL);
    type.set_scale(DEFAULT_SCALE_FOR_INTEGER);
    type.set_calc_type(type1.get_calc_type());
    type.set_result_flag(NOT_NULL_FLAG);
    type2.set_calc_type(type2.get_type());
  } else {
    // always allow NULL value
    type.set_result_flag(NOT_NULL_FLAG);
    //is operator第二个参数必须保持原来的NULL或者FALSE, TRUE
    type2.set_calc_type(type2.get_type());
    type3.set_calc_type(type3.get_type());

    ObRawExpr *param2 = op_expr->get_param_expr(1);
    ObConstRawExpr *const_param2 = static_cast<ObConstRawExpr *>(param2);
    ObRawExpr *param3 = op_expr->get_param_expr(2);
    ObConstRawExpr *const_param3 = static_cast<ObConstRawExpr *>(param3);
    if (OB_ISNULL(const_param2) || OB_ISNULL(const_param3)) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("second or third child of is expr is null", K(ret), K(const_param2),
                                                            K(const_param3));
    } else if (ObDoubleType == const_param2->get_value().get_type()) {
        type1.set_calc_type(ObDoubleType);
        type1.set_calc_accuracy(type1.get_accuracy());
    } else if (!const_param2->get_value().is_null()) {  // is true/false
      if (ob_is_numeric_type(type1.get_type())) {
        type1.set_calc_meta(type1.get_obj_meta());
        type1.set_calc_accuracy(type1.get_accuracy());
      } else {
        type1.set_calc_type(ObNumberType);
        const ObAccuracy &calc_acc = ObAccuracy::DDL_DEFAULT_ACCURACY2[0][ObNumberType];
        type1.set_calc_accuracy(calc_acc);
      }
    } else {    // is null
      if (const_param3->get_value().is_true()) {
        type1.set_calc_type(ObIntType);
      } else {
        type1.set_calc_type(type1.get_type());
      }
      // query range extract check the calc type of %type.
      type.set_calc_meta(type1.get_calc_meta());
    }
    type.set_type(ObInt32Type);
    type.set_precision(DEFAULT_PRECISION_FOR_BOOL);
    type.set_scale(DEFAULT_SCALE_FOR_INTEGER);
  }
  type_ctx.set_cast_mode(type_ctx.get_cast_mode() | CM_NO_RANGE_CHECK);

  return ret;
}

int ObExprIs::calc_with_null(common::ObObj &result,
                             const ObObj &obj1,
                             const ObObj &obj2,
                             const ObObj &obj3,
                             ObCastCtx &cast_ctx) const
{
  int ret = OB_SUCCESS;
  bool ret_bool = false;
  UNUSED(obj2);
  UNUSED(cast_ctx);
  if (obj3.is_true()) {
    if(obj1.is_null()) {
      ret_bool = true;
    } else {
      int64_t int_val = 0;
      EXPR_GET_INT64_V2(obj1, int_val);
      if (OB_SUCC(ret)) {
        if (int_val == 0) {
      ret_bool = true;
        } else {
      ret_bool = false;
        }
      }
    }
  } else {
    if (lib::is_oracle_mode() && obj1.get_meta().is_ext()) {
      switch (obj1.get_meta().get_extend_type()) {
        case pl::PL_RECORD_TYPE: {
          pl::ObPLRecord *rec = reinterpret_cast<pl::ObPLRecord *>(obj1.get_ext());
          ret_bool = rec != NULL ? rec->is_null() : true;
        }
          break;
        default: {
          ret = OB_NOT_SUPPORTED;
          LOG_WARN("check complex value is null not supported", K(ret), K(obj1));
          LOG_USER_ERROR(OB_NOT_SUPPORTED, "check complex is null");
        } break;
      }
    } else {
      ret_bool = obj1.is_null();
    }
  }

  if (OB_SUCC(ret)) {
    result.set_int32(static_cast<int32_t>(ret_bool));
  }

  return ret;
}

int ObExprIsNot::calc_with_null(ObObj &result,
                                const ObObj &obj1,
                                const ObObj &obj2,
                                const ObObj &obj3,
                                ObCastCtx &cast_ctx) const
{
  int ret = OB_SUCCESS;
  UNUSED(obj2);
  UNUSED(obj3);
  UNUSED(cast_ctx);
  bool ret_bool = false;
  if (lib::is_oracle_mode() && obj1.get_meta().is_ext()) {
    switch (obj1.get_meta().get_extend_type()) {
      default: {
        ret = OB_NOT_SUPPORTED;
        LOG_WARN("check complex value is null not supported", K(ret), K(obj1), K(obj1.get_meta().get_extend_type()));
        LOG_USER_ERROR(OB_NOT_SUPPORTED, "check complex is null");
      } break;
    }
  } else {
    ret_bool = !obj1.is_null();
  }
  result.set_int32(static_cast<int32_t>(ret_bool));
  return ret;
}

int ObExprIs::calc_collection_is_null(const ObExpr &expr, ObEvalCtx &ctx, ObDatum &expr_datum)
{
  int ret = OB_SUCCESS;
  ObDatum *param = NULL;
  if (OB_FAIL(expr.args_[0]->eval(ctx, param))) {
    LOG_WARN("evaluate parameter failed", K(ret));
  } else {
    bool v = false;
    if (param->is_null() || param->extend_obj_->is_null()) {
      v = true;
    } else {
      uint64_t ext = param->extend_obj_->get_ext();
      switch (param->extend_obj_->get_meta().get_extend_type()) {
        case pl::PL_RECORD_TYPE: {
          pl::ObPLRecord *rec = reinterpret_cast<pl::ObPLRecord *>(ext);
          v = rec->is_null();
          break;
        }
        default: {
          ret = OB_NOT_SUPPORTED;
          LOG_WARN("check complex value is null not supported", K(ret), K(param->extend_obj_));
          LOG_USER_ERROR(OB_NOT_SUPPORTED, "check complex is null");
        } break;
      }
    }
    if (OB_SUCC(ret)) {
      expr_datum.set_int32(v);
    }
  }
  return ret;
}

int ObExprIsNot::calc_collection_is_not_null(const ObExpr &expr,
                                             ObEvalCtx &ctx,
                                             ObDatum &expr_datum)
{
  int ret = OB_SUCCESS;
  if (OB_FAIL(ObExprIs::calc_collection_is_null(expr, ctx, expr_datum))) {
    LOG_WARN("eval is null failed", K(ret));
  } else {
    ObDatum &d = expr.locate_expr_datum(ctx);
    d.set_int32(!d.get_int32());
  }
  return ret;
}


int ObExprIsBase::calc_with_int_internal(ObObj &result,
                                         const ObObj &obj1,
                                         const ObObj &obj2,
                                         ObCastCtx &cast_ctx,
                                         bool is_not) const
{
  int ret = OB_SUCCESS;
  bool ret_bool = false;
  if (obj1.is_null()) {
    ret_bool = is_not;
  } else if (obj1.is_tinyint()) {
    ret_bool = is_not ?
                    obj1.get_bool() != obj2.get_bool() :
                    obj1.get_bool() == obj2.get_bool();
  } else {
    int64_t int_val = 0;
    EXPR_GET_INT64_V2(obj1, int_val);
    if (OB_SUCC(ret)) {
      if ((int_val != 0 && obj2.get_bool() == is_not)
          || (int_val == 0 && obj2.get_bool() == !is_not)) {
        ret_bool = false;
      } else {
        ret_bool = true;
      }
    } else {
      ret = OB_SUCCESS;
      if (obj2.get_bool() == is_not) {
        ret_bool = true;
      } else {
        ret_bool = false;
      }
    }
  }
  if (OB_SUCC(ret)) {
    result.set_int32(static_cast<int32_t>(ret_bool));
  }
  return ret;
}

int ObExprIsBase::is_infinite_nan(const ObObjType datum_type,
                              ObDatum *datum,
                              bool &ret_bool,
                              Ieee754 inf_or_nan)
{
  int ret = OB_SUCCESS;
  if (inf_or_nan == Ieee754::INFINITE_VALUE) {
    if (ObFloatType == datum_type) {
      ret_bool = (fabs(datum->get_float()) == INFINITY ? true: false);
    } else if (ObDoubleType == datum_type) {
      ret_bool = (fabs(datum->get_double()) == INFINITY ? true: false);
    } else {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("is_infinite: invalid column type", K(datum_type));
    }
  } else {
    if (ObFloatType == datum_type) {
      ret_bool = isnan(fabs(datum->get_float()));
    } else if (ObDoubleType == datum_type) {
      ret_bool = isnan(fabs(datum->get_double()));
    } else {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("is_infinite: invalid column type", K(datum_type));
    }
  }

  return ret;
}

int ObExprIs::calc_with_int(ObObj &result,
                            const ObObj &obj1,
                            const ObObj &obj2,
                            ObCastCtx &cast_ctx) const
{
  return calc_with_int_internal(result, obj1, obj2, cast_ctx, false);
}

int ObExprIsNot::calc_with_int(ObObj &result,
                               const ObObj &obj1,
                               const ObObj &obj2,
                               ObCastCtx &cast_ctx) const
{
  return calc_with_int_internal(result, obj1, obj2, cast_ctx, true);
}

int ObExprIs::calc_with_nan(ObObj &result,
                            const ObObj &obj1,
                            const ObObj &obj2,
                            ObCastCtx &cast_ctx) const
{
  return calc_with_nan_internal(result, obj1, obj2, cast_ctx, false);
}

int ObExprIs::calc_with_infinity(ObObj &result,
                            const ObObj &obj1,
                            const ObObj &obj2,
                            ObCastCtx &cast_ctx) const
{
  return calc_with_infinity_internal(result, obj1, obj2, cast_ctx, false);
}

int ObExprIsBase::calc_with_nan_internal(ObObj &result,
                                         const ObObj &obj1,
                                         const ObObj &obj2,
                                         ObCastCtx &cast_ctx,
                                         bool is_not) const
{
  UNUSED(obj2);
  UNUSED(cast_ctx);
  int ret = OB_SUCCESS;
  bool ret_bool = false;
  if (obj1.is_null()) {
    result.set_null();
  } else {
    if (obj1.is_double()) {
      ret_bool = is_not ? !isnan(obj1.get_double()) : isnan(obj1.get_double());
    } else {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("Unexpected case", K(obj1.get_type()));
    }
    if (OB_SUCC(ret)) {
      result.set_int32(static_cast<int32_t>(ret_bool));
    }
  }
  return ret;
}

int ObExprIsBase::calc_with_infinity_internal(ObObj &result,
                                         const ObObj &obj1,
                                         const ObObj &obj2,
                                         ObCastCtx &cast_ctx,
                                         bool is_not) const
{
  UNUSED(obj2);
  UNUSED(cast_ctx);
  int ret = OB_SUCCESS;
  bool ret_bool = false;
  if (obj1.is_null()) {
    result.set_null();
  } else {
    if (obj1.is_double()) {
      ret_bool = is_not ? !isinf(obj1.get_double()) : isinf(obj1.get_double());
    } else {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("Unexpected case", K(obj1.get_type()));
    }
    if (OB_SUCC(ret)) {
      result.set_int32(static_cast<int32_t>(ret_bool));
    }
  }
  return ret;
}

int ObExprIsBase::cg_expr_internal(ObExprCGCtx &op_cg_ctx, const ObRawExpr &raw_expr,
                                   ObExpr &rt_expr, const ObConstRawExpr *&const_param2,
                                   const ObConstRawExpr *&const_param3) const
{
 	UNUSED(op_cg_ctx);
  int ret = OB_SUCCESS;
  const ObOpRawExpr *op_raw_expr = static_cast<const ObOpRawExpr*>(&raw_expr);
  if (rt_expr.arg_cnt_ != 3) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("isnot expr should have 3 params", K(ret), K(rt_expr.arg_cnt_));
  } else if (OB_ISNULL(rt_expr.args_) || OB_ISNULL(rt_expr.args_[0])
            || OB_ISNULL(rt_expr.args_[1]) || OB_ISNULL(rt_expr.args_[2])) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("children of isnot expr is null", K(ret), K(rt_expr.args_));
  } else {
    const ObRawExpr *param2 = op_raw_expr->get_param_expr(1);
    const ObRawExpr *param3 = op_raw_expr->get_param_expr(2);
    const_param2 = static_cast<const ObConstRawExpr *>(param2);
    const_param3 = static_cast<const ObConstRawExpr *>(param3);
  }
  return ret;
}

#define EVAL_FUNC(type)                         \
  if (is_not && is_true) {                      \
    eval_func = ObExprIsNot::type##_is_not_true;  \
  } else if (is_not && !is_true) {              \
    eval_func = ObExprIsNot::type##_is_not_false; \
  } else if (!is_not && is_true) {              \
    eval_func = ObExprIs::type##_is_true;         \
  } else if (!is_not && !is_true) {             \
    eval_func = ObExprIs::type##_is_false;        \
  }

int ObExprIsBase::cg_result_type_class(ObObjType type, ObExpr::EvalFunc &eval_func, bool is_not,
                                        bool is_true) const {
  int ret = OB_SUCCESS;
  switch (type) {
    case ObTinyIntType:
    case ObSmallIntType:
    case ObMediumIntType:
    case ObInt32Type:
    case ObIntType:
    case ObUTinyIntType:
    case ObUSmallIntType:
    case ObUMediumIntType:
    case ObUInt32Type:
    case ObUInt64Type:
    case ObBitType: {
        EVAL_FUNC(int);
        break;
    }
    case ObFloatType:
    case ObUFloatType:{
        EVAL_FUNC(float);
        break;
    }
    case ObDoubleType:
    case ObUDoubleType: {
        EVAL_FUNC(double);
        break;
    }
    case ObMaxType: {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("is expr got unexpected type param", K(ret), K(type));
        break;
    }
    default: {
        EVAL_FUNC(number);
        break;
    }
  }
  return ret;
}

int ObExprIs::cg_expr(ObExprCGCtx &op_cg_ctx, const ObRawExpr &raw_expr, ObExpr &rt_expr) const
{
  int ret = OB_SUCCESS;
  const ObConstRawExpr *param2 = NULL;
  const ObConstRawExpr *param3 = NULL;
  ObObjType param1_type = ObMaxType;
  if (OB_FAIL(cg_expr_internal(op_cg_ctx, raw_expr, rt_expr, param2, param3))) {
    LOG_WARN("cg_expr_inner failed", K(ret));
  } else if (OB_ISNULL(param2) || OB_ISNULL(param3)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("const raw expr param2 or param3 is null", K(param2), K(param3));
  } else if(FALSE_IT(param1_type = rt_expr.args_[0]->datum_meta_.type_)) {
  } else if (param3->get_value().is_true()) { // 特殊情况，not null的date或datetime列
    rt_expr.eval_func_ = ObExprIs::calc_is_date_int_null;
  } else if (param2->get_value().is_null()) {  // c1 is null
    if (lib::is_oracle_mode() && rt_expr.args_[0]->obj_meta_.is_ext()) {
      rt_expr.eval_func_ = ObExprIs::calc_collection_is_null;
    } else {
      rt_expr.eval_func_ = ObExprIs::calc_is_null;
    }
  } else if (param2->get_value().is_true()) {
    if (OB_FAIL(cg_result_type_class(param1_type, rt_expr.eval_func_, false, true))) {
      LOG_WARN("is expr got unexpected type param", K(ret));
    }
  } else if (param2->get_value().is_false()) {
    if (OB_FAIL(cg_result_type_class(param1_type, rt_expr.eval_func_, false, false))) {
      LOG_WARN("is expr got unexpected type param", K(ret));
    }
  } else if (ObDoubleType == param2->get_value().get_type()) {
    if (isnan(param2->get_value().get_double())) {
      rt_expr.eval_func_ = ObExprIs::calc_is_nan;
    } else {
      rt_expr.eval_func_ = ObExprIs::calc_is_infinite;
    }
  } else {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("second param of is expr is not null, true, false or infinite or nan",
              K(ret), K(param2->get_value()));
  }
  return ret;
}

int ObExprIsNot::cg_expr(ObExprCGCtx &op_cg_ctx, const ObRawExpr &raw_expr, ObExpr &rt_expr) const
{
  int ret = OB_SUCCESS;
  const ObConstRawExpr *param2 = NULL;
  const ObConstRawExpr *param3 = NULL;
  ObObjType param1_type = ObMaxType;
  if (OB_FAIL(cg_expr_internal(op_cg_ctx, raw_expr, rt_expr, param2, param3))) {
    LOG_WARN("cg_expr_inner failed", K(ret));
  } else if (OB_ISNULL(param2) || OB_ISNULL(param3)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("const raw expr param2 or param3 is null", K(param2), K(param3));
  } else if(FALSE_IT(param1_type = rt_expr.args_[0]->datum_meta_.type_)) {
  } else if (param2->get_value().is_null()) {  // c1 is null
    if (lib::is_oracle_mode() && rt_expr.args_[0]->obj_meta_.is_ext()) {
      rt_expr.eval_func_ = ObExprIsNot::calc_collection_is_not_null;
    } else {
      rt_expr.eval_func_ = ObExprIsNot::calc_is_not_null;
    }
  } else if (param2->get_value().is_true()) {
    if (OB_FAIL(cg_result_type_class(param1_type, rt_expr.eval_func_, true, true))) {
      LOG_WARN("is expr got unexpected type param", K(ret));
    }
  } else if (param2->get_value().is_false()) {
    if (OB_FAIL(cg_result_type_class(param1_type, rt_expr.eval_func_, true, false))) {
      LOG_WARN("is expr got unexpected type param", K(ret));
    }
  } else if (ObDoubleType == param2->get_value().get_type()) {
    if (isnan(param2->get_value().get_double())) {
      rt_expr.eval_func_ = ObExprIsNot::calc_is_not_nan;
    } else {
      rt_expr.eval_func_ = ObExprIsNot::calc_is_not_infinite;
    }
  } else {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("second param of is expr is not null, true, false or infinite or nan",
                K(ret), K(param2->get_value()));
  }
  return ret;
}

int ObExprIs::calc_is_date_int_null(const ObExpr &expr, ObEvalCtx &ctx, ObDatum &expr_datum)
{
  int ret = OB_SUCCESS;
  ObDatum *param1 = NULL;
  if (OB_FAIL(expr.args_[0]->eval(ctx, param1))) {
    LOG_WARN("eval first param failed", K(ret));
  } else {
    bool ret_bool = param1->is_null() || param1->get_int() == 0;
    expr_datum.set_int32(static_cast<int32_t>(ret_bool));
  }
  return ret;
}

int ObExprIs::calc_is_null(const ObExpr &expr, ObEvalCtx &ctx, ObDatum &expr_datum)
{
  int ret = OB_SUCCESS;
  ObDatum *param1 = NULL;
  if (OB_FAIL(expr.args_[0]->eval(ctx, param1))) {
    LOG_WARN("eval first param failed", K(ret));
  } else {
    bool ret_bool = param1->is_null();
    expr_datum.set_int32(static_cast<int32_t>(ret_bool));
  }
  return ret;
}

int ObExprIs::calc_is_infinite(const ObExpr &expr, ObEvalCtx &ctx, ObDatum &expr_datum)
{
  int ret = OB_SUCCESS;
  ObDatum *param1 = NULL;
  bool ret_bool = false;
  if (OB_FAIL(expr.args_[0]->eval(ctx, param1))) {
    LOG_WARN("eval first param failed", K(ret));
  } else if (param1->is_null()) {
    expr_datum.set_null();
  } else {
    if (OB_FAIL(is_infinite_nan(expr.args_[0]->datum_meta_.type_, param1,
                                ret_bool, Ieee754::INFINITE_VALUE))) {
      LOG_WARN("calc_is_infinite unexpect error", K(ret));
    } else {
      expr_datum.set_int32(static_cast<int32_t>(ret_bool));
    }
  }
  return ret;
}

int ObExprIs::calc_is_nan(const ObExpr &expr, ObEvalCtx &ctx, ObDatum &expr_datum)
{
  int ret = OB_SUCCESS;
  ObDatum *param1 = NULL;
  bool ret_bool = false;
  if (OB_FAIL(expr.args_[0]->eval(ctx, param1))) {
    LOG_WARN("eval first param failed", K(ret));
  } else if (param1->is_null()) {
    expr_datum.set_null();
  } else {
    if (OB_FAIL(is_infinite_nan(expr.args_[0]->datum_meta_.type_, param1,
                                ret_bool, Ieee754::NAN_VALUE))) {
      LOG_WARN("calc_is_nan unexpect error", K(ret));
    } else {
      expr_datum.set_int32(static_cast<int32_t>(ret_bool));
    }
  }
  return ret;
}

int ObExprIsNot::calc_is_not_null(const ObExpr &expr, ObEvalCtx &ctx, ObDatum &expr_datum)
{
  int ret = OB_SUCCESS;
  ObDatum *param1 = NULL;
  if (OB_FAIL(expr.args_[0]->eval(ctx, param1))) {
    LOG_WARN("eval first param failed", K(ret));
  } else {
    bool ret_bool = !param1->is_null();
    expr_datum.set_int32(static_cast<int32_t>(ret_bool));
  }
  return ret;
}

int ObExprIsNot::calc_is_not_infinite(const ObExpr &expr, ObEvalCtx &ctx, ObDatum &expr_datum)
{
  int ret = OB_SUCCESS;
  ObDatum *param1 = NULL;
  bool ret_bool = false;
  if (OB_FAIL(expr.args_[0]->eval(ctx, param1))) {
    LOG_WARN("eval first param failed", K(ret));
  } else if (param1->is_null()) {
    expr_datum.set_null();
  } else {
    if (OB_FAIL(is_infinite_nan(expr.args_[0]->datum_meta_.type_, param1,
                                ret_bool, Ieee754::INFINITE_VALUE))) {
      LOG_WARN("calc_is_not_infinite unexpect error", K(ret));
    } else {
      expr_datum.set_int32(static_cast<int32_t>(!ret_bool));
    }
  }
  return ret;
}

int ObExprIsNot::calc_is_not_nan(const ObExpr &expr, ObEvalCtx &ctx, ObDatum &expr_datum)
{
  int ret = OB_SUCCESS;
  ObDatum *param1 = NULL;
  if (OB_FAIL(expr.args_[0]->eval(ctx, param1))) {
    LOG_WARN("eval first param failed", K(ret));
  } else if (param1->is_null()) {
    expr_datum.set_null();
  } else {
    bool ret_bool = false;
    if (OB_FAIL(is_infinite_nan(expr.args_[0]->datum_meta_.type_, param1,
                                ret_bool, Ieee754::NAN_VALUE))) {
      LOG_WARN("calc_is_not_nan unexpect error", K(ret));
    } else {
      expr_datum.set_int32(static_cast<int32_t>(!ret_bool));
    }
  }
  return ret;
}

int ObExprIsNot::calc_with_nan(ObObj &result,
                            const ObObj &obj1,
                            const ObObj &obj2,
                            ObCastCtx &cast_ctx) const
{
  return calc_with_nan_internal(result, obj1, obj2, cast_ctx, true);
}

int ObExprIsNot::calc_with_infinity(ObObj &result,
                            const ObObj &obj1,
                            const ObObj &obj2,
                            ObCastCtx &cast_ctx) const
{
  return calc_with_infinity_internal(result, obj1, obj2, cast_ctx, true);
}

template <typename T>
int ObExprIsBase::is_zero(T number)
{
  return 0 == number;
}
template <>
int ObExprIsBase::is_zero<number::ObCompactNumber>(number::ObCompactNumber number)
{
  return number.is_zero();
}

#define NUMERIC_CALC_FUNC(type, func_name, bool_is_not, is_true, str_is_not)                \
  int func_name::type##_##str_is_not##_##is_true(const ObExpr &expr, ObEvalCtx &ctx,        \
                                                ObDatum &expr_datum)                    \
  {                                                           \
    int ret = OB_SUCCESS;                                     \
    ObDatum *param1 = NULL;                                   \
    bool ret_bool = false;                                    \
    if (OB_FAIL(expr.args_[0]->eval(ctx, param1))) {          \
      LOG_WARN("eval first param failed", K(ret));            \
    } else if (param1->is_null()) {                           \
      ret_bool = bool_is_not;                                 \
    } else {                                                  \
      ret_bool = bool_is_not == is_true ?                     \
                is_zero(param1->get_##type()) :               \
                !is_zero(param1->get_##type());               \
    }                                                         \
    if (OB_SUCC(ret)) {                                       \
      expr_datum.set_int32(static_cast<int32_t>(ret_bool));   \
    }                                                         \
    return ret;                                               \
  }

NUMERIC_CALC_FUNC(int, ObExprIsNot, true, true, is_not)
NUMERIC_CALC_FUNC(float, ObExprIsNot, true, true, is_not)
NUMERIC_CALC_FUNC(double, ObExprIsNot, true, true, is_not)
NUMERIC_CALC_FUNC(number, ObExprIsNot, true, true, is_not)

NUMERIC_CALC_FUNC(int, ObExprIsNot, true, false, is_not)
NUMERIC_CALC_FUNC(float, ObExprIsNot, true, false, is_not)
NUMERIC_CALC_FUNC(double, ObExprIsNot, true, false, is_not)
NUMERIC_CALC_FUNC(number, ObExprIsNot, true, false, is_not)

NUMERIC_CALC_FUNC(int, ObExprIs, false, true, is)
NUMERIC_CALC_FUNC(float, ObExprIs, false, true, is)
NUMERIC_CALC_FUNC(double, ObExprIs, false, true, is)
NUMERIC_CALC_FUNC(number, ObExprIs, false, true, is)

NUMERIC_CALC_FUNC(int, ObExprIs, false, false, is)
NUMERIC_CALC_FUNC(float, ObExprIs, false, false, is)
NUMERIC_CALC_FUNC(double, ObExprIs, false, false, is)
NUMERIC_CALC_FUNC(number, ObExprIs, false, false, is)
}
}
