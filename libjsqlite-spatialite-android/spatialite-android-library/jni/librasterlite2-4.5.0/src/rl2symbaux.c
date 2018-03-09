/*

 rl2symbaux -- private SQL helper methods

 version 0.1, 2015 January 18

 Author: Sandro Furieri a.furieri@lqt.it

 -----------------------------------------------------------------------------
 
 Version: MPL 1.1/GPL 2.0/LGPL 2.1
 
 The contents of this file are subject to the Mozilla Public License Version
 1.1 (the "License"); you may not use this file except in compliance with
 the License. You may obtain a copy of the License at
 http://www.mozilla.org/MPL/
 
Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
for the specific language governing rights and limitations under the
License.

The Original Code is the RasterLite2 library

The Initial Developer of the Original Code is Alessandro Furieri
 
Portions created by the Initial Developer are Copyright (C) 2008-2013
the Initial Developer. All Rights Reserved.

Alternatively, the contents of this file may be used under the terms of
either the GNU General Public License Version 2 or later (the "GPL"), or
the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
in which case the provisions of the GPL or the LGPL are applicable instead
of those above. If you wish to allow use of your version of this file only
under the terms of either the GPL or the LGPL, and not to allow others to
use your version of this file under the terms of the MPL, indicate your
decision by deleting the provisions above and replace them with the notice
and other provisions required by the GPL or the LGPL. If you do not delete
the provisions above, a recipient may use your version of this file under
the terms of any one of the MPL, the GPL or the LGPL.
 
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <limits.h>
#include <stdint.h>
#include <inttypes.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "config.h"

#include <libxml/parser.h>

#ifdef LOADABLE_EXTENSION
#include "rasterlite2/sqlite.h"
#endif

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2_private.h"

#define RL2_UNUSED() if (argc || argv) argc = argc;

RL2_PRIVATE rl2PrivRasterSymbolizerPtr
rl2_create_default_raster_symbolizer ()
{
/* creating a default Raster Style */
    rl2PrivRasterSymbolizerPtr symbolizer =
	malloc (sizeof (rl2PrivRasterSymbolizer));
    if (symbolizer == NULL)
	return NULL;
    symbolizer->opacity = 1.0;
    symbolizer->bandSelection = NULL;
    symbolizer->categorize = NULL;
    symbolizer->interpolate = NULL;
    symbolizer->contrastEnhancement = RL2_CONTRAST_ENHANCEMENT_NONE;
    symbolizer->gammaValue = 1.0;
    symbolizer->shadedRelief = 0;
    symbolizer->brightnessOnly = 0;
    symbolizer->reliefFactor = 55.0;
    return symbolizer;
}

RL2_PRIVATE rl2PrivStyleRulePtr
rl2_create_default_style_rule ()
{
/* creating a default (empty) Style Rule */
    rl2PrivStyleRulePtr rule = malloc (sizeof (rl2PrivStyleRule));
    if (rule == NULL)
	return NULL;
    rule->else_rule = 0;
    rule->min_scale = DBL_MAX;
    rule->max_scale = DBL_MAX;
    rule->comparison_op = RL2_COMPARISON_NONE;
    rule->comparison_args = NULL;
    rule->column_name = NULL;
    rule->style_type = RL2_UNKNOWN_STYLE;
    rule->style = NULL;
    rule->next = NULL;
    return rule;
}

RL2_PRIVATE rl2PrivRuleSingleArgPtr
rl2_create_default_rule_single_arg ()
{
/* creating default (empty) Rule Single Arg */
    rl2PrivRuleSingleArgPtr arg = malloc (sizeof (rl2PrivRuleSingleArg));
    if (arg == NULL)
	return NULL;
    arg->value = NULL;
    return arg;
}

RL2_PRIVATE rl2PrivRuleLikeArgsPtr
rl2_create_default_rule_like_args ()
{
/* creating default (empty) Rule Like Args */
    rl2PrivRuleLikeArgsPtr args = malloc (sizeof (rl2PrivRuleLikeArgs));
    if (args == NULL)
	return NULL;
    args->wild_card = NULL;
    args->single_char = NULL;
    args->escape_char = NULL;
    args->value = NULL;
    return args;
}

RL2_PRIVATE rl2PrivRuleBetweenArgsPtr
rl2_create_default_rule_between_args ()
{
/* creating default (empty) Rule Between Args */
    rl2PrivRuleBetweenArgsPtr args = malloc (sizeof (rl2PrivRuleBetweenArgs));
    if (args == NULL)
	return NULL;
    args->lower = NULL;
    args->upper = NULL;
    return args;
}

RL2_PRIVATE rl2PrivVectorSymbolizerPtr
rl2_create_default_vector_symbolizer ()
{
/* creating a default Vector Style */
    rl2PrivVectorSymbolizerPtr symbolizer =
	malloc (sizeof (rl2PrivVectorSymbolizer));
    if (symbolizer == NULL)
	return NULL;
    symbolizer->first = NULL;
    symbolizer->last = NULL;
    return symbolizer;
}

RL2_PRIVATE rl2PrivCoverageStylePtr
rl2_create_default_coverage_style ()
{
/* creating a default (empty) Coverage Style */
    rl2PrivCoverageStylePtr style = malloc (sizeof (rl2PrivCoverageStyle));
    if (style == NULL)
	return NULL;
    style->name = NULL;
    style->first_rule = NULL;
    style->last_rule = NULL;
    return style;
}

RL2_DECLARE void
rl2_destroy_coverage_style (rl2CoverageStylePtr style)
{
/* destroying a CoverageStyle object */
    rl2PrivStyleRulePtr pR;
    rl2PrivStyleRulePtr pRn;
    rl2PrivCoverageStylePtr stl = (rl2PrivCoverageStylePtr) style;
    if (stl == NULL)
	return;

    if (stl->name != NULL)
	free (stl->name);

    pR = stl->first_rule;
    while (pR != NULL)
      {
	  pRn = pR->next;
	  rl2_destroy_style_rule (pR);
	  pR = pRn;
      }
    free (stl);
}

RL2_DECLARE const char *
rl2_get_coverage_style_name (rl2CoverageStylePtr style)
{
/* return the CoverageStyle Name */
    rl2PrivCoverageStylePtr stl = (rl2PrivCoverageStylePtr) style;
    if (stl == NULL)
	return NULL;
    return stl->name;
}

RL2_DECLARE rl2RasterSymbolizerPtr
rl2_get_symbolizer_from_coverage_style (rl2CoverageStylePtr style, double scale)
{
/* return the RasterSymbolizer matching a given scale from a CoverageStyle */
    rl2PrivRasterSymbolizerPtr symbolizer = NULL;
    rl2PrivStyleRulePtr pR;
    rl2PrivCoverageStylePtr stl = (rl2PrivCoverageStylePtr) style;
    if (stl == NULL)
	return NULL;

    pR = stl->first_rule;
    while (pR != NULL)
      {
	  if (pR->style_type == RL2_RASTER_STYLE && pR->style != NULL)
	      ;
	  else
	    {
		/* skipping any invalid rule */
		pR = pR->next;
		continue;
	    }
	  if (pR->min_scale != DBL_MAX && pR->max_scale != DBL_MAX)
	    {
		if (scale >= pR->min_scale && scale < pR->max_scale)
		  {
		      symbolizer = pR->style;
		      break;
		  }
	    }
	  else if (pR->min_scale != DBL_MAX)
	    {
		if (scale >= pR->min_scale)
		  {
		      symbolizer = pR->style;
		      break;
		  }
	    }
	  else if (pR->max_scale != DBL_MAX)
	    {
		if (scale < pR->max_scale)
		  {
		      symbolizer = pR->style;
		      break;
		  }
	    }
	  else
	    {
		symbolizer = pR->style;
		break;
	    }
	  pR = pR->next;
      }
    return (rl2RasterSymbolizerPtr) symbolizer;
}

RL2_DECLARE void
rl2_destroy_feature_type_style (rl2FeatureTypeStylePtr style)
{
/* destroying a FeatureTypeStyle object */
    int i;
    rl2PrivStyleRulePtr pR;
    rl2PrivStyleRulePtr pRn;
    rl2PrivFeatureTypeStylePtr stl = (rl2PrivFeatureTypeStylePtr) style;
    if (stl == NULL)
	return;

    if (stl->name != NULL)
	free (stl->name);

    pR = stl->first_rule;
    while (pR != NULL)
      {
	  pRn = pR->next;
	  rl2_destroy_style_rule (pR);
	  pR = pRn;
      }
    if (stl->else_rule != NULL)
	rl2_destroy_style_rule (stl->else_rule);
    if (stl->column_names != NULL)
      {
	  for (i = 0; i < stl->columns_count; i++)
	    {
		if (*(stl->column_names + i) != NULL)
		    free (*(stl->column_names + i));
	    }
	  free (stl->column_names);
      }
    free (stl);
}

RL2_DECLARE const char *
rl2_get_feature_type_style_name (rl2FeatureTypeStylePtr style)
{
/* return the FeatureTypeStyle Name */
    rl2PrivFeatureTypeStylePtr stl = (rl2PrivFeatureTypeStylePtr) style;
    if (stl == NULL)
	return NULL;
    return stl->name;
}

RL2_DECLARE int
rl2_get_feature_type_style_columns_count (rl2FeatureTypeStylePtr style)
{
/* return the FeatureTypeStyle ColumnsCount */
    rl2PrivFeatureTypeStylePtr stl = (rl2PrivFeatureTypeStylePtr) style;
    if (stl == NULL)
	return 0;
    if (stl->column_names == NULL)
	return 0;
    return stl->columns_count;
}

RL2_DECLARE const char *
rl2_get_feature_type_style_column_name (rl2FeatureTypeStylePtr style, int index)
{
/* return the Nth FeatureTypeStyle ColumnName */
    rl2PrivFeatureTypeStylePtr stl = (rl2PrivFeatureTypeStylePtr) style;
    if (stl == NULL)
	return NULL;
    if (stl->column_names != NULL && index >= 0 && index < stl->columns_count)
	return *(stl->column_names + index);
    return NULL;
}

static int
is_valid_numeric_literal (const char *literal)
{
/* checking if a literal could be of the numeric type */
    const char *in = literal;
    while (1)
      {
	  /* skipping all whitespaces */
	  if (*in == ' ' || *in == '\t')
	      in++;
	  else
	      break;
      }
    if (*in == '-' || *in == '+')
      {
	  /* leading sign */
	  in++;
      }
    while (*in != '\0')
      {
	  if (*in == '.')
	    {
		/* decimal point */
		in++;
		break;
	    }
	  if (*in >= '0' && *in <= '9')
	    {
		/* digit */
		in++;
		continue;
	    }
	  return 0;
      }
    while (*in != '\0')
      {
	  if (*in >= '0' && *in <= '9')
	    {
		/* digit */
		in++;
		continue;
	    }
	  return 0;
      }
    return 1;
}

static int
eval_filter_eq (rl2PrivStyleRulePtr rule, rl2PrivVariantValuePtr val)
{
/* evaluating an IsEqual comparison */
    sqlite3_int64 intval;
    double dblval;
    rl2PrivRuleSingleArgPtr arg =
	(rl2PrivRuleSingleArgPtr) (rule->comparison_args);
    if (arg == NULL)
	return 0;
    switch (val->sqlite3_type)
      {
      case SQLITE_INTEGER:
	  if (is_valid_numeric_literal (arg->value))
	    {
		intval = atoll (arg->value);
		if (intval == val->int_value)
		    return 1;
	    }
	  break;
      case SQLITE_FLOAT:
	  if (is_valid_numeric_literal (arg->value))
	    {
		dblval = atof (arg->value);
		if (dblval == val->dbl_value)
		    return 1;
	    }
	  break;
      case SQLITE_TEXT:
	  if (strcmp (arg->value, val->text_value) == 0)
	      return 1;
	  break;
      };
    return 0;
}

static int
eval_filter_ne (rl2PrivStyleRulePtr rule, rl2PrivVariantValuePtr val)
{
/* evaluating an IsNotEqual comparison */
    sqlite3_int64 intval;
    double dblval;
    rl2PrivRuleSingleArgPtr arg =
	(rl2PrivRuleSingleArgPtr) (rule->comparison_args);
    if (arg == NULL)
	return 1;
    switch (val->sqlite3_type)
      {
      case SQLITE_INTEGER:
	  if (is_valid_numeric_literal (arg->value))
	    {
		intval = atoll (arg->value);
		if (intval == val->int_value)
		    return 0;
	    }
	  break;
      case SQLITE_FLOAT:
	  if (is_valid_numeric_literal (arg->value))
	    {
		dblval = atof (arg->value);
		if (dblval == val->dbl_value)
		    return 0;
	    }
	  break;
      case SQLITE_TEXT:
	  if (is_valid_numeric_literal (arg->value))
	    {
		if (strcmp (arg->value, val->text_value) == 0)
		    return 0;
	    }
	  break;
      case SQLITE_BLOB:
      case SQLITE_NULL:
	  return 0;
	  break;
      };
    return 1;
}

static int
eval_filter_lt (rl2PrivStyleRulePtr rule, rl2PrivVariantValuePtr val)
{
/* evaluating an IsLessThan comparison */
    sqlite3_int64 intval;
    double dblval;
    rl2PrivRuleSingleArgPtr arg =
	(rl2PrivRuleSingleArgPtr) (rule->comparison_args);
    if (arg == NULL)
	return 0;
    switch (val->sqlite3_type)
      {
      case SQLITE_INTEGER:
	  if (is_valid_numeric_literal (arg->value))
	    {
		intval = atoll (arg->value);
		if (val->int_value < intval)
		    return 1;
	    }
	  break;
      case SQLITE_FLOAT:
	  if (is_valid_numeric_literal (arg->value))
	    {
		dblval = atof (arg->value);
		if (val->dbl_value < dblval)
		    return 1;
	    }
	  break;
      case SQLITE_TEXT:
	  if (strcmp (val->text_value, arg->value) < 0)
	      return 1;
	  break;
      };
    return 0;
}

static int
eval_filter_gt (rl2PrivStyleRulePtr rule, rl2PrivVariantValuePtr val)
{
/* evaluating an IsGreaterThan comparison */
    sqlite3_int64 intval;
    double dblval;
    rl2PrivRuleSingleArgPtr arg =
	(rl2PrivRuleSingleArgPtr) (rule->comparison_args);
    if (arg == NULL)
	return 0;
    switch (val->sqlite3_type)
      {
      case SQLITE_INTEGER:
	  if (is_valid_numeric_literal (arg->value))
	    {
		intval = atoll (arg->value);
		if (val->int_value > intval)
		    return 1;
	    }
	  break;
      case SQLITE_FLOAT:
	  if (is_valid_numeric_literal (arg->value))
	    {
		dblval = atof (arg->value);
		if (val->dbl_value > dblval)
		    return 1;
	    }
	  break;
      case SQLITE_TEXT:
	  if (strcmp (val->text_value, arg->value) > 0)
	      return 1;
	  break;
      };
    return 0;
}

static int
eval_filter_le (rl2PrivStyleRulePtr rule, rl2PrivVariantValuePtr val)
{
/* evaluating an IsLessThanOrEqualTo comparison */
    sqlite3_int64 intval;
    double dblval;
    rl2PrivRuleSingleArgPtr arg =
	(rl2PrivRuleSingleArgPtr) (rule->comparison_args);
    if (arg == NULL)
	return 0;
    switch (val->sqlite3_type)
      {
      case SQLITE_INTEGER:
	  if (is_valid_numeric_literal (arg->value))
	    {
		intval = atoll (arg->value);
		if (val->int_value <= intval)
		    return 1;
	    }
	  break;
      case SQLITE_FLOAT:
	  if (is_valid_numeric_literal (arg->value))
	    {
		dblval = atof (arg->value);
		if (val->dbl_value <= dblval)
		    return 1;
	    }
	  break;
      case SQLITE_TEXT:
	  if (strcmp (val->text_value, arg->value) <= 0)
	      return 1;
	  break;
      };
    return 0;
}

static int
eval_filter_ge (rl2PrivStyleRulePtr rule, rl2PrivVariantValuePtr val)
{
/* evaluating an IsGreaterThanOrEqualTo comparison */
    sqlite3_int64 intval;
    double dblval;
    rl2PrivRuleSingleArgPtr arg =
	(rl2PrivRuleSingleArgPtr) (rule->comparison_args);
    if (arg == NULL)
	return 0;
    switch (val->sqlite3_type)
      {
      case SQLITE_INTEGER:
	  if (is_valid_numeric_literal (arg->value))
	    {
		intval = atoll (arg->value);
		if (val->int_value >= intval)
		    return 1;
	    }
	  break;
      case SQLITE_FLOAT:
	  if (is_valid_numeric_literal (arg->value))
	    {
		dblval = atof (arg->value);
		if (val->dbl_value >= dblval)
		    return 1;
	    }
	  break;
      case SQLITE_TEXT:
	  if (strcmp (val->text_value, arg->value) >= 0)
	      return 1;
	  break;
      };
    return 0;
}

static int
eval_filter_between (rl2PrivStyleRulePtr rule, rl2PrivVariantValuePtr val)
{
/* evaluating a Between comparison */
    sqlite3_int64 int_lo;
    sqlite3_int64 int_hi;
    double dbl_lo;
    double dbl_hi;
    rl2PrivRuleBetweenArgsPtr arg =
	(rl2PrivRuleBetweenArgsPtr) (rule->comparison_args);
    if (arg == NULL)
	return 0;
    switch (val->sqlite3_type)
      {
      case SQLITE_INTEGER:
	  if (is_valid_numeric_literal (arg->lower)
	      && is_valid_numeric_literal (arg->upper))
	    {
		int_lo = atoll (arg->lower);
		int_hi = atoll (arg->upper);
		if (val->int_value >= int_lo && val->int_value < int_hi)
		    return 1;
	    }
	  break;
      case SQLITE_FLOAT:
	  if (is_valid_numeric_literal (arg->lower)
	      && is_valid_numeric_literal (arg->upper))
	    {
		dbl_lo = atof (arg->lower);
		dbl_hi = atof (arg->upper);
		if (val->dbl_value >= dbl_lo && val->dbl_value < dbl_hi)
		    return 1;
	    }
	  break;
      case SQLITE_TEXT:
	  if (strcmp (val->text_value, arg->lower) >= 0
	      && strcmp (val->text_value, arg->upper) < 0)
	      return 1;
	  break;
      };
    return 0;
}

static int
eval_filter_like (rl2PrivStyleRulePtr rule, rl2PrivVariantValuePtr val)
{
/* evaluating a Like comparison */
    char *intval;
    char *extval;
    char *out;
    char wild_card;
    char single_char;
    char escape_char;
    char special;
    char pending;
    const char *in;
    const char *intptr;
    const char *extptr;
    int len;
    int escape = 0;
    int mismatch = 0;
    int i;
    rl2PrivRuleLikeArgsPtr arg =
	(rl2PrivRuleLikeArgsPtr) (rule->comparison_args);
    if (arg == NULL)
	return 0;
    if (val->sqlite3_type != SQLITE_TEXT)
	return 0;

    wild_card = *(arg->wild_card);
    single_char = *(arg->single_char);
    escape_char = *(arg->escape_char);

    extval = malloc (val->bytes + 1);
    strcpy (extval, val->text_value);
    for (i = 0; i < val->bytes; i++)
      {
	  /* transforming the external value into lowercase */
	  if (*(extval + i) >= 'A' && *(extval + i) <= 'Z')
	      *(extval + i) = *(extval + i) - 'A' + 'a';
      }
    extptr = extval;

    len = strlen (arg->value);
    intval = malloc (len + 1);

    pending = '\0';
    intptr = arg->value;
    while (*intptr != '\0')
      {
	  /* identifying the substring */
	  out = intval;
	  in = intptr;
	  special = '\0';
	  while (*in != '\0')
	    {
		if (escape)
		  {
		      *out++ = *in++;
		      escape = 0;
		      continue;
		  }
		if (*in == escape_char)
		  {
		      in++;
		      escape = 1;
		      continue;
		  }
		if (*in == wild_card)
		  {
		      special = *in++;
		      intptr = in;
		      break;
		  }
		if (*in == single_char)
		  {
		      special = *in++;
		      intptr = in;
		      break;
		  }
		if (*in >= 'A' && *in <= 'Z')
		    *out++ = *in++ - 'A' + 'a';
		else
		    *out++ = *in++;
	    }
	  *out = '\0';
	  /* substring comparison */
	  if (*extptr == '\0')
	    {
		mismatch = 1;
		goto end;
	    }
	  if (pending == wild_card)
	    {
		char *ptr;
		if (*intptr == '\0' && special != wild_card)
		    break;
		ptr = strstr (extptr, intval);
		if (ptr == NULL)
		  {
		      mismatch = 1;
		      goto end;
		  }
		len = ptr - extptr;
		len += strlen (intval);
		extptr += len;
		if (special != wild_card)
		    intptr += strlen (intval);
	    }
	  else
	    {
		len = strlen (intval);
		if (strncmp (intval, extptr, len) != 0)
		  {
		      mismatch = 1;
		      goto end;
		  }
		extptr += len;
	    }
	  if (special == single_char)
	      extptr++;
	  else if (special == wild_card)
	      pending = wild_card;
	  else
	      pending = '\0';
      }
    if (pending == wild_card)
      {
	  /* ignoring the final input substring */
	  while (*extptr != '\0')
	      extptr++;
      }
    if (*extptr != '\0')
	mismatch = 1;

  end:
    free (extval);
    free (intval);
    if (mismatch)
	return 0;
    return 1;
}

static int
eval_filter (rl2PrivStyleRulePtr rule, rl2VariantArrayPtr variant)
{
/* evaluating a Rule Filter */
    int i;
    rl2PrivVariantArrayPtr var = (rl2PrivVariantArrayPtr) variant;
    if (rule->column_name == NULL)
	return 1;		/* there is no comparison: surely true */
    if (var == NULL)
	return 0;
    for (i = 0; i < var->count; i++)
      {
	  rl2PrivVariantValuePtr val = *(var->array + i);
	  if (val == NULL)
	      return 0;
	  if (val->column_name == NULL)
	      return 0;
	  if (strcasecmp (rule->column_name, val->column_name) != 0)
	      continue;
	  switch (rule->comparison_op)
	    {
	    case RL2_COMPARISON_EQ:
		return eval_filter_eq (rule, val);
	    case RL2_COMPARISON_NE:
		return eval_filter_ne (rule, val);
	    case RL2_COMPARISON_LT:
		return eval_filter_lt (rule, val);
	    case RL2_COMPARISON_GT:
		return eval_filter_gt (rule, val);
	    case RL2_COMPARISON_LE:
		return eval_filter_le (rule, val);
	    case RL2_COMPARISON_GE:
		return eval_filter_ge (rule, val);
	    case RL2_COMPARISON_LIKE:
		return eval_filter_like (rule, val);
	    case RL2_COMPARISON_BETWEEN:
		return eval_filter_between (rule, val);
	    case RL2_COMPARISON_NULL:
		if (val->sqlite3_type == SQLITE_NULL)
		    return 1;
		break;
	    };
	  break;
      }
    return 0;
}

RL2_DECLARE rl2VectorSymbolizerPtr
rl2_get_symbolizer_from_feature_type_style (rl2FeatureTypeStylePtr style,
					    double scale,
					    rl2VariantArrayPtr variant,
					    int *scale_forbidden)
{
/* return the VectorSymbolizer matching a given scale/filter from a FeatureTypeStyle */
    rl2PrivVectorSymbolizerPtr symbolizer = NULL;
    rl2PrivStyleRulePtr pR;
    rl2PrivFeatureTypeStylePtr stl = (rl2PrivFeatureTypeStylePtr) style;
    *scale_forbidden = 0;
    if (stl == NULL)
	return NULL;

    pR = stl->first_rule;
    while (pR != NULL)
      {
	  if (pR->style_type == RL2_VECTOR_STYLE && pR->style != NULL)
	      ;
	  else
	    {
		/* skipping any invalid rule */
		pR = pR->next;
		continue;
	    }

	  if (eval_filter (pR, variant))
	    {
		*scale_forbidden = 0;
		if (pR->min_scale != DBL_MAX && pR->max_scale != DBL_MAX)
		  {
		      if (scale >= pR->min_scale && scale < pR->max_scale)
			  symbolizer = pR->style;
		  }
		else if (pR->min_scale != DBL_MAX)
		  {
		      if (scale >= pR->min_scale)
			  symbolizer = pR->style;
		  }
		else if (pR->max_scale != DBL_MAX)
		  {
		      if (scale < pR->max_scale)
			  symbolizer = pR->style;
		  }
		else
		    symbolizer = pR->style;
		if (symbolizer == NULL)
		    *scale_forbidden = 1;
		else
		    return (rl2VectorSymbolizerPtr) symbolizer;
	    }
	  pR = pR->next;
      }

    pR = stl->else_rule;
    if (pR != NULL)
      {
	  if (pR->min_scale != DBL_MAX && pR->max_scale != DBL_MAX)
	    {
		if (scale >= pR->min_scale && scale < pR->max_scale)
		    symbolizer = pR->style;
	    }
	  else if (pR->min_scale != DBL_MAX)
	    {
		if (scale >= pR->min_scale)
		    symbolizer = pR->style;
	    }
	  else if (pR->max_scale != DBL_MAX)
	    {
		if (scale < pR->max_scale)
		    symbolizer = pR->style;
	    }
	  else
	      symbolizer = pR->style;
	  if (symbolizer == NULL)
	      *scale_forbidden = 1;
      }
    return (rl2VectorSymbolizerPtr) symbolizer;
}

RL2_DECLARE int
rl2_is_visible_style (rl2FeatureTypeStylePtr style, double scale)
{
/* test visibility at a given scale/filter from a FeatureTypeStyle */
    int count = 0;
    int visible;
    rl2PrivStyleRulePtr pR;
    rl2PrivFeatureTypeStylePtr stl = (rl2PrivFeatureTypeStylePtr) style;
    if (stl == NULL)
	return 0;
    if (stl->first_rule == NULL)
      {
	  /* there are no rules: unconditional visibility */
	  return 1;
      }

    pR = stl->first_rule;
    while (pR != NULL)
      {
	  if (pR->style_type == RL2_VECTOR_STYLE && pR->style != NULL)
	      ;
	  else
	    {
		/* skipping any invalid rule */
		pR = pR->next;
		continue;
	    }
	  visible = 1;
	  if (pR->min_scale != DBL_MAX && pR->max_scale != DBL_MAX)
	    {
		visible = 0;
		if (scale >= pR->min_scale && scale < pR->max_scale)
		    visible = 1;
	    }
	  else if (pR->min_scale != DBL_MAX)
	    {
		visible = 0;
		if (scale >= pR->min_scale)
		    visible = 1;
	    }
	  else if (pR->max_scale != DBL_MAX)
	    {
		visible = 0;
		if (scale < pR->max_scale)
		    visible = 1;
	    }
	  if (visible)
	      count++;
	  pR = pR->next;
      }
    if (count == 0)
	return 0;
    return 1;
}

RL2_DECLARE int
rl2_style_has_labels (rl2FeatureTypeStylePtr style)
{
/* test if a FeatureTypeStyle has Text Symbolizers */
    rl2PrivVectorSymbolizerPtr sym;
    rl2PrivVectorSymbolizerItemPtr item;
    rl2PrivStyleRulePtr pR;
    rl2PrivFeatureTypeStylePtr stl = (rl2PrivFeatureTypeStylePtr) style;
    if (stl == NULL)
	return 0;

    pR = stl->first_rule;
    while (pR != NULL)
      {
	  if (pR->style_type == RL2_VECTOR_STYLE && pR->style != NULL)
	      ;
	  else
	    {
		/* skipping any invalid rule */
		pR = pR->next;
		continue;
	    }
	  sym = pR->style;
	  item = sym->first;
	  while (item != NULL)
	    {
		if (item->symbolizer_type == RL2_TEXT_SYMBOLIZER
		    && item->symbolizer != NULL)
		  {
		      /* found a valid Text Symbolizer */
		      return 1;
		  }
		item = item->next;
	    }
	  pR = pR->next;
      }

    pR = stl->else_rule;
    if (pR != NULL)
      {
	  if (pR->style_type == RL2_VECTOR_STYLE && pR->style != NULL)
	      ;
	  else
	      return 0;
	  sym = pR->style;
	  item = sym->first;
	  while (item != NULL)
	    {
		if (item->symbolizer_type == RL2_TEXT_SYMBOLIZER
		    && item->symbolizer != NULL)
		  {
		      /* found a valid Text Symbolizer */
		      return 1;
		  }
		item = item->next;
	    }
      }
    return 0;
}

RL2_PRIVATE void
rl2_destroy_rule_like_args (rl2PrivRuleLikeArgsPtr args)
{
/* destroying a Rule Like arguments object */
    if (args == NULL)
	return;

    if (args->wild_card != NULL)
	free (args->wild_card);
    if (args->single_char != NULL)
	free (args->single_char);
    if (args->escape_char != NULL)
	free (args->escape_char);
    if (args->value != NULL)
	free (args->value);
    free (args);
}

RL2_PRIVATE void
rl2_destroy_rule_between_args (rl2PrivRuleBetweenArgsPtr args)
{
/* destroying a Rule Between arguments object */
    if (args == NULL)
	return;

    if (args->lower != NULL)
	free (args->lower);
    if (args->upper != NULL)
	free (args->upper);
    free (args);
}

RL2_PRIVATE void
rl2_destroy_rule_single_arg (rl2PrivRuleSingleArgPtr arg)
{
/* destroying a Rule Single argument object */
    if (arg == NULL)
	return;

    if (arg->value != NULL)
	free (arg->value);
    free (arg);
}

RL2_PRIVATE void
rl2_destroy_style_rule (rl2PrivStyleRulePtr rule)
{
/* destroying a StyleRule object */
    if (rule == NULL)
	return;

    if (rule->column_name != NULL)
	free (rule->column_name);
    if (rule->comparison_args != NULL)
      {
	  if (rule->comparison_op == RL2_COMPARISON_LIKE)
	      rl2_destroy_rule_like_args (rule->comparison_args);
	  else if (rule->comparison_op == RL2_COMPARISON_BETWEEN)
	      rl2_destroy_rule_between_args (rule->comparison_args);
	  else
	      rl2_destroy_rule_single_arg (rule->comparison_args);
      }
    if (rule->style != NULL)
      {
	  if (rule->style_type == RL2_VECTOR_STYLE)
	      rl2_destroy_vector_symbolizer (rule->style);
	  if (rule->style_type == RL2_RASTER_STYLE)
	      rl2_destroy_raster_symbolizer (rule->style);
      }
    free (rule);
}

RL2_PRIVATE void
rl2_destroy_raster_symbolizer (rl2PrivRasterSymbolizerPtr stl)
{
/* destroying a RasterSymbolizer object */
    rl2PrivColorMapPointPtr pC;
    rl2PrivColorMapPointPtr pCn;
    if (stl == NULL)
	return;

    if (stl->bandSelection != NULL)
	free (stl->bandSelection);
    if (stl->categorize != NULL)
      {
	  pC = stl->categorize->first;
	  while (pC != NULL)
	    {
		pCn = pC->next;
		free (pC);
		pC = pCn;
	    }
	  free (stl->categorize);
      }
    if (stl->interpolate != NULL)
      {
	  pC = stl->interpolate->first;
	  while (pC != NULL)
	    {
		pCn = pC->next;
		free (pC);
		pC = pCn;
	    }
	  free (stl->interpolate);
      }
    free (stl);
}

RL2_DECLARE int
rl2_get_raster_symbolizer_opacity (rl2RasterSymbolizerPtr style,
				   double *opacity)
{
/* return the RasterSymbolizer Opacity */
    rl2PrivRasterSymbolizerPtr stl = (rl2PrivRasterSymbolizerPtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    *opacity = stl->opacity;
    return RL2_OK;
}

RL2_DECLARE int
rl2_is_raster_symbolizer_mono_band_selected (rl2RasterSymbolizerPtr style,
					     int *selected, int *categorize,
					     int *interpolate)
{
/* return if the RasterSymbolizer has a MonoBand selection */
    rl2PrivRasterSymbolizerPtr stl = (rl2PrivRasterSymbolizerPtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (stl->shadedRelief)
      {
	  /* Shaded Relief */
	  *selected = 1;
	  *categorize = 0;
	  *interpolate = 0;
	  return RL2_OK;
      }
    if (stl->bandSelection == NULL)
      {
	  if (stl->categorize != NULL)
	    {
		/* Categorize Color Map */
		*selected = 1;
		*categorize = 1;
		*interpolate = 0;
		return RL2_OK;
	    }
	  if (stl->interpolate != NULL)
	    {
		/* Interpolate Color Map */
		*selected = 1;
		*categorize = 0;
		*interpolate = 1;
		return RL2_OK;
	    }
	  if (stl->contrastEnhancement == RL2_CONTRAST_ENHANCEMENT_NORMALIZE
	      || stl->contrastEnhancement ==
	      RL2_CONTRAST_ENHANCEMENT_HISTOGRAM
	      || stl->contrastEnhancement == RL2_CONTRAST_ENHANCEMENT_GAMMA)
	    {
		/* Contrast Enhancement */
		*selected = 1;
		*categorize = 0;
		*interpolate = 0;
		return RL2_OK;
	    }
      }
    if (stl->bandSelection == NULL)
	*selected = 0;
    else if (stl->bandSelection->selectionType == RL2_BAND_SELECTION_MONO)
	*selected = 1;
    else
	*selected = 0;
    *categorize = 0;
    *interpolate = 0;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_raster_symbolizer_mono_band_selection (rl2RasterSymbolizerPtr style,
					       unsigned char *gray_band)
{
/* return the RasterSymbolizer MonoBand selection */
    rl2PrivRasterSymbolizerPtr stl = (rl2PrivRasterSymbolizerPtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (stl->bandSelection == NULL)
      {
	  if (stl->categorize != NULL)
	    {
		/* Categorize Color Map */
		*gray_band = 0;
		return RL2_OK;
	    }
	  if (stl->interpolate != NULL)
	    {
		/* Interpolate Color Map */
		*gray_band = 0;
		return RL2_OK;
	    }
	  /* Interpolate Color Map */
	  *gray_band = 0;
	  return RL2_OK;
      }
    if (stl->bandSelection == NULL)
	return RL2_ERROR;
    else if (stl->bandSelection->selectionType == RL2_BAND_SELECTION_MONO)
      {
	  *gray_band = stl->bandSelection->grayBand;
	  return RL2_OK;
      }
    else
	return RL2_ERROR;
}

RL2_DECLARE int
rl2_is_raster_symbolizer_triple_band_selected (rl2RasterSymbolizerPtr style,
					       int *selected)
{
/* return if the RasterSymbolizer has a TripleBand selection */
    rl2PrivRasterSymbolizerPtr stl = (rl2PrivRasterSymbolizerPtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (stl->bandSelection == NULL)
      {
	  if (stl->contrastEnhancement == RL2_CONTRAST_ENHANCEMENT_NORMALIZE
	      || stl->contrastEnhancement ==
	      RL2_CONTRAST_ENHANCEMENT_HISTOGRAM
	      || stl->contrastEnhancement == RL2_CONTRAST_ENHANCEMENT_GAMMA)
	    {
		/* Contrast Enhancement */
		*selected = 1;
		return RL2_OK;
	    }
      }
    if (stl->bandSelection == NULL)
	*selected = 0;
    else if (stl->bandSelection->selectionType == RL2_BAND_SELECTION_TRIPLE)
	*selected = 1;
    else
	*selected = 0;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_raster_symbolizer_triple_band_selection (rl2RasterSymbolizerPtr style,
						 unsigned char *red_band,
						 unsigned char *green_band,
						 unsigned char *blue_band)
{
/* return the RasterSymbolizer TripleBand selection */
    rl2PrivRasterSymbolizerPtr stl = (rl2PrivRasterSymbolizerPtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (stl->bandSelection == NULL)
      {
	  if (stl->contrastEnhancement == RL2_CONTRAST_ENHANCEMENT_NORMALIZE
	      || stl->contrastEnhancement ==
	      RL2_CONTRAST_ENHANCEMENT_HISTOGRAM
	      || stl->contrastEnhancement == RL2_CONTRAST_ENHANCEMENT_GAMMA)
	    {
		/* Contrast Enhancement */
		*red_band = 0;
		*green_band = 1;
		*blue_band = 2;
		return RL2_OK;
	    }
      }
    if (stl->bandSelection == NULL)
	return RL2_ERROR;
    else if (stl->bandSelection->selectionType == RL2_BAND_SELECTION_TRIPLE)
      {
	  *red_band = stl->bandSelection->redBand;
	  *green_band = stl->bandSelection->greenBand;
	  *blue_band = stl->bandSelection->blueBand;
	  return RL2_OK;
      }
    else
	return RL2_ERROR;
}

RL2_DECLARE int
rl2_get_raster_symbolizer_overall_contrast_enhancement (rl2RasterSymbolizerPtr
							style,
							unsigned char
							*contrast_enhancement,
							double *gamma_value)
{
/* return the RasterSymbolizer OverallContrastEnhancement */
    rl2PrivRasterSymbolizerPtr stl = (rl2PrivRasterSymbolizerPtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    *contrast_enhancement = stl->contrastEnhancement;
    *gamma_value = stl->gammaValue;
    return RL2_OK;
}

RL2_DECLARE int
    rl2_get_raster_symbolizer_red_band_contrast_enhancement
    (rl2RasterSymbolizerPtr style, unsigned char *contrast_enhancement,
     double *gamma_value)
{
/* return the RasterSymbolizer RedBand ContrastEnhancement */
    rl2PrivRasterSymbolizerPtr stl = (rl2PrivRasterSymbolizerPtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (stl->bandSelection == NULL)
	return RL2_ERROR;
    else if (stl->bandSelection->selectionType == RL2_BAND_SELECTION_TRIPLE)
      {
	  *contrast_enhancement = stl->bandSelection->redContrast;
	  *gamma_value = stl->bandSelection->redGamma;
	  return RL2_OK;
      }
    return RL2_ERROR;
}

RL2_DECLARE int
    rl2_get_raster_symbolizer_green_band_contrast_enhancement
    (rl2RasterSymbolizerPtr style, unsigned char *contrast_enhancement,
     double *gamma_value)
{
/* return the RasterSymbolizer GreenBand ContrastEnhancement */
    rl2PrivRasterSymbolizerPtr stl = (rl2PrivRasterSymbolizerPtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (stl->bandSelection == NULL)
	return RL2_ERROR;
    else if (stl->bandSelection->selectionType == RL2_BAND_SELECTION_TRIPLE)
      {
	  *contrast_enhancement = stl->bandSelection->greenContrast;
	  *gamma_value = stl->bandSelection->greenGamma;
	  return RL2_OK;
      }
    return RL2_ERROR;
}

RL2_DECLARE int
    rl2_get_raster_symbolizer_blue_band_contrast_enhancement
    (rl2RasterSymbolizerPtr style, unsigned char *contrast_enhancement,
     double *gamma_value)
{
/* return the RasterSymbolizer BlueBand ContrastEnhancement */
    rl2PrivRasterSymbolizerPtr stl = (rl2PrivRasterSymbolizerPtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (stl->bandSelection == NULL)
	return RL2_ERROR;
    else if (stl->bandSelection->selectionType == RL2_BAND_SELECTION_TRIPLE)
      {
	  *contrast_enhancement = stl->bandSelection->blueContrast;
	  *gamma_value = stl->bandSelection->blueGamma;
	  return RL2_OK;
      }
    return RL2_ERROR;
}

RL2_DECLARE int
    rl2_get_raster_symbolizer_gray_band_contrast_enhancement
    (rl2RasterSymbolizerPtr style, unsigned char *contrast_enhancement,
     double *gamma_value)
{
/* return the RasterSymbolizer GrayBand ContrastEnhancement */
    rl2PrivRasterSymbolizerPtr stl = (rl2PrivRasterSymbolizerPtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (stl->bandSelection == NULL)
	return RL2_ERROR;
    else if (stl->bandSelection->selectionType == RL2_BAND_SELECTION_MONO)
      {
	  *contrast_enhancement = stl->bandSelection->grayContrast;
	  *gamma_value = stl->bandSelection->grayGamma;
	  return RL2_OK;
      }
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_has_raster_symbolizer_shaded_relief (rl2RasterSymbolizerPtr style,
					 int *shaded_relief)
{
/* return if the RasterSymbolizer has ShadedRelief */
    rl2PrivRasterSymbolizerPtr stl = (rl2PrivRasterSymbolizerPtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    *shaded_relief = stl->shadedRelief;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_raster_symbolizer_shaded_relief (rl2RasterSymbolizerPtr style,
					 int *brightness_only,
					 double *relief_factor)
{
/* return the RasterSymbolizer ShadedRelief parameters */
    rl2PrivRasterSymbolizerPtr stl = (rl2PrivRasterSymbolizerPtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (stl->shadedRelief)
      {
	  *brightness_only = stl->brightnessOnly;
	  *relief_factor = stl->reliefFactor;
	  return RL2_OK;
      }
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_has_raster_symbolizer_color_map_interpolated (rl2RasterSymbolizerPtr
						  style, int *interpolated)
{
/* return if the RasterSymbolizer has an Interpolated ColorMap */
    rl2PrivRasterSymbolizerPtr stl = (rl2PrivRasterSymbolizerPtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (stl->interpolate != NULL)
	*interpolated = 1;
    else
	*interpolated = 0;
    return RL2_OK;
}

RL2_DECLARE int
rl2_has_raster_symbolizer_color_map_categorized (rl2RasterSymbolizerPtr style,
						 int *categorized)
{
/* return if the RasterSymbolizer has a Categorized ColorMap */
    rl2PrivRasterSymbolizerPtr stl = (rl2PrivRasterSymbolizerPtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (stl->categorize != NULL)
	*categorized = 1;
    else
	*categorized = 0;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_raster_symbolizer_color_map_default (rl2RasterSymbolizerPtr style,
					     unsigned char *red,
					     unsigned char *green,
					     unsigned char *blue)
{
/* return the RasterSymbolizer ColorMap Default color */
    rl2PrivRasterSymbolizerPtr stl = (rl2PrivRasterSymbolizerPtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (stl->interpolate != NULL)
      {
	  *red = stl->interpolate->dfltRed;
	  *green = stl->interpolate->dfltGreen;
	  *blue = stl->interpolate->dfltBlue;
	  return RL2_OK;
      }
    if (stl->categorize != NULL)
      {
	  *red = stl->categorize->dfltRed;
	  *green = stl->categorize->dfltGreen;
	  *blue = stl->categorize->dfltBlue;
	  return RL2_OK;
      }
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_get_raster_symbolizer_color_map_category_base (rl2RasterSymbolizerPtr
						   style, unsigned char *red,
						   unsigned char *green,
						   unsigned char *blue)
{
/* return the RasterSymbolizer ColorMap Category base-color */
    rl2PrivRasterSymbolizerPtr stl = (rl2PrivRasterSymbolizerPtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (stl->categorize != NULL)
      {
	  *red = stl->categorize->baseRed;
	  *green = stl->categorize->baseGreen;
	  *blue = stl->categorize->baseBlue;
	  return RL2_OK;
      }
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_get_raster_symbolizer_color_map_count (rl2RasterSymbolizerPtr style,
					   int *count)
{
/* return the RasterSymbolizer ColorMap items count */
    int cnt;
    rl2PrivColorMapPointPtr pt;
    rl2PrivRasterSymbolizerPtr stl = (rl2PrivRasterSymbolizerPtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (stl->categorize != NULL)
      {
	  cnt = 0;
	  pt = stl->categorize->first;
	  while (pt != NULL)
	    {
		cnt++;
		pt = pt->next;
	    }
	  *count = cnt;
	  return RL2_OK;
      }
    if (stl->interpolate != NULL)
      {
	  cnt = 0;
	  pt = stl->interpolate->first;
	  while (pt != NULL)
	    {
		cnt++;
		pt = pt->next;
	    }
	  *count = cnt;
	  return RL2_OK;
      }
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_get_raster_symbolizer_color_map_entry (rl2RasterSymbolizerPtr style,
					   int index, double *value,
					   unsigned char *red,
					   unsigned char *green,
					   unsigned char *blue)
{
/* return the RasterSymbolizer ColorMap item values */
    int cnt;
    rl2PrivColorMapPointPtr pt;
    rl2PrivRasterSymbolizerPtr stl = (rl2PrivRasterSymbolizerPtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (stl->categorize != NULL)
      {
	  cnt = 0;
	  pt = stl->categorize->first;
	  while (pt != NULL)
	    {
		if (index == cnt)
		  {
		      *value = pt->value;
		      *red = pt->red;
		      *green = pt->green;
		      *blue = pt->blue;
		      return RL2_OK;
		  }
		cnt++;
		pt = pt->next;
	    }
      }
    if (stl->interpolate != NULL)
      {
	  cnt = 0;
	  pt = stl->interpolate->first;
	  while (pt != NULL)
	    {
		if (index == cnt)
		  {
		      *value = pt->value;
		      *red = pt->red;
		      *green = pt->green;
		      *blue = pt->blue;
		      return RL2_OK;
		  }
		cnt++;
		pt = pt->next;
	    }
      }
    return RL2_ERROR;
}

RL2_PRIVATE rl2PrivColorReplacementPtr
rl2_create_default_color_replacement ()
{
/* creating a default Color Replacement object */
    rl2PrivColorReplacementPtr repl = malloc (sizeof (rl2PrivColorReplacement));
    repl->index = 0;
    repl->red = 0;
    repl->green = 0;
    repl->blue = 0;
    repl->col_color = NULL;
    repl->next = NULL;
    return repl;
}

RL2_PRIVATE rl2PrivGraphicItemPtr
rl2_create_default_external_graphic ()
{
/* creating a default Graphic Item object (ExternalGraphic) */
    rl2PrivGraphicItemPtr item = malloc (sizeof (rl2PrivGraphicItem));
    rl2PrivExternalGraphicPtr ext = malloc (sizeof (rl2PrivExternalGraphic));
    ext->xlink_href = NULL;
    ext->col_href = NULL;
    ext->first = NULL;
    ext->last = NULL;
    item->type = RL2_EXTERNAL_GRAPHIC;
    item->item = ext;
    item->next = NULL;
    return item;
}

RL2_PRIVATE rl2PrivGraphicItemPtr
rl2_create_default_mark ()
{
/* creating a default Graphic Item object (Mark) */
    rl2PrivGraphicItemPtr item = malloc (sizeof (rl2PrivGraphicItem));
    rl2PrivMarkPtr mark = malloc (sizeof (rl2PrivMark));
    mark->well_known_type = RL2_GRAPHIC_MARK_UNKNOWN;
    mark->stroke = NULL;
    mark->fill = NULL;
    mark->col_mark_type = NULL;
    item->type = RL2_MARK_GRAPHIC;
    item->item = mark;
    item->next = NULL;
    return item;
}

RL2_PRIVATE rl2PrivGraphicPtr
rl2_create_default_graphic ()
{
/* creating a default Graphic object) */
    rl2PrivGraphicPtr graphic = malloc (sizeof (rl2PrivGraphic));
    graphic->first = NULL;
    graphic->last = NULL;
    graphic->opacity = 1.0;
    graphic->size = 6.0;
    graphic->rotation = 0.0;
    graphic->anchor_point_x = 0.5;
    graphic->anchor_point_y = 0.5;
    graphic->displacement_x = 0.0;
    graphic->displacement_y = 0.0;
    graphic->col_opacity = NULL;
    graphic->col_size = NULL;
    graphic->col_rotation = NULL;
    graphic->col_point_x = NULL;
    graphic->col_point_y = NULL;
    graphic->col_displ_x = NULL;
    graphic->col_displ_y = NULL;
    return graphic;
}

RL2_PRIVATE rl2PrivStrokePtr
rl2_create_default_stroke ()
{
/* creating a default Stroke object */
    rl2PrivStrokePtr stroke = malloc (sizeof (rl2PrivStroke));
    stroke->graphic = NULL;
    stroke->red = 0;
    stroke->green = 0;
    stroke->blue = 0;
    stroke->opacity = 1.0;
    stroke->width = 1.0;
    stroke->linejoin = RL2_STROKE_LINEJOIN_UNKNOWN;
    stroke->linecap = RL2_STROKE_LINECAP_UNKNOWN;
    stroke->dash_count = 0;
    stroke->dash_list = NULL;
    stroke->dash_offset = 0.0;
    stroke->col_color = NULL;
    stroke->col_opacity = NULL;
    stroke->col_width = NULL;
    stroke->col_join = NULL;
    stroke->col_cap = NULL;
    stroke->col_dash = NULL;
    stroke->col_dashoff = NULL;
    return stroke;
}

RL2_PRIVATE rl2PrivPointPlacementPtr
rl2_create_default_point_placement ()
{
/* creating a default PointPlacement object */
    rl2PrivPointPlacementPtr place = malloc (sizeof (rl2PrivPointPlacement));
    place->anchor_point_x = 0.5;
    place->anchor_point_y = 0.5;
    place->displacement_x = 0.0;
    place->displacement_y = 0.0;
    place->rotation = 0.0;
    place->col_point_x = NULL;
    place->col_point_y = NULL;
    place->col_displ_x = NULL;
    place->col_displ_y = NULL;
    place->col_rotation = NULL;
    return place;
}

RL2_PRIVATE rl2PrivLinePlacementPtr
rl2_create_default_line_placement ()
{
/* creating a default LinePlacement object */
    rl2PrivLinePlacementPtr place = malloc (sizeof (rl2PrivLinePlacement));
    place->perpendicular_offset = 0.0;
    place->is_repeated = 0;
    place->initial_gap = 0.0;
    place->gap = 0.0;
    place->generalize_line = 0;
    place->col_perpoff = NULL;
    place->col_inigap = NULL;
    place->col_gap = NULL;
    return place;
}

RL2_PRIVATE rl2PrivFillPtr
rl2_create_default_fill ()
{
/* creating a default Fill object */
    rl2PrivFillPtr fill = malloc (sizeof (rl2PrivFill));
    fill->graphic = NULL;
    fill->red = 128;
    fill->green = 128;
    fill->blue = 128;
    fill->opacity = 1.0;
    fill->col_color = NULL;
    fill->col_opacity = NULL;
    return fill;
}

RL2_PRIVATE rl2PrivHaloPtr
rl2_create_default_halo ()
{
/* creating a default Halo object */
    rl2PrivHaloPtr halo = malloc (sizeof (rl2PrivHalo));
    halo->radius = 1.0;
    halo->fill = NULL;
    halo->col_radius = NULL;
    return halo;
}

RL2_PRIVATE rl2PrivVectorSymbolizerItemPtr
rl2_create_default_point_symbolizer ()
{
/* creating a default Point Symbolizer */
    rl2PrivVectorSymbolizerItemPtr item =
	malloc (sizeof (rl2PrivVectorSymbolizerItem));
    rl2PrivPointSymbolizerPtr symbolizer =
	malloc (sizeof (rl2PrivPointSymbolizer));
    if (symbolizer == NULL || item == NULL)
      {
	  if (symbolizer != NULL)
	      free (symbolizer);
	  if (item != NULL)
	      free (item);
	  return NULL;
      }
    symbolizer->graphic = NULL;
    item->symbolizer_type = RL2_POINT_SYMBOLIZER;
    item->symbolizer = symbolizer;
    item->next = NULL;
    return item;
}

RL2_PRIVATE rl2PrivVectorSymbolizerItemPtr
rl2_create_default_line_symbolizer ()
{
/* creating a default Line Symbolizer */
    rl2PrivVectorSymbolizerItemPtr item =
	malloc (sizeof (rl2PrivVectorSymbolizerItem));
    rl2PrivLineSymbolizerPtr symbolizer =
	malloc (sizeof (rl2PrivLineSymbolizer));
    if (symbolizer == NULL || item == NULL)
      {
	  if (symbolizer != NULL)
	      free (symbolizer);
	  if (item != NULL)
	      free (item);
	  return NULL;
      }
    symbolizer->stroke = NULL;
    symbolizer->perpendicular_offset = 0.0;
    symbolizer->col_perpoff = NULL;
    item->symbolizer_type = RL2_LINE_SYMBOLIZER;
    item->symbolizer = symbolizer;
    item->next = NULL;
    return item;
}

RL2_PRIVATE rl2PrivVectorSymbolizerItemPtr
rl2_create_default_polygon_symbolizer ()
{
/* creating a default Polygon Symbolizer */
    rl2PrivVectorSymbolizerItemPtr item =
	malloc (sizeof (rl2PrivVectorSymbolizerItem));
    rl2PrivPolygonSymbolizerPtr symbolizer =
	malloc (sizeof (rl2PrivPolygonSymbolizer));
    if (symbolizer == NULL || item == NULL)
      {
	  if (symbolizer != NULL)
	      free (symbolizer);
	  if (item != NULL)
	      free (item);
	  return NULL;
      }
    symbolizer->stroke = NULL;
    symbolizer->fill = NULL;
    symbolizer->displacement_x = 0.0;
    symbolizer->displacement_y = 0.0;
    symbolizer->perpendicular_offset = 0.0;
    symbolizer->col_displ_x = NULL;
    symbolizer->col_displ_y = NULL;
    symbolizer->col_perpoff = NULL;
    item->symbolizer_type = RL2_POLYGON_SYMBOLIZER;
    item->symbolizer = symbolizer;
    item->next = NULL;
    return item;
}

RL2_PRIVATE rl2PrivVectorSymbolizerItemPtr
rl2_create_default_text_symbolizer ()
{
/* creating a default Text Symbolizer */
    int i;
    rl2PrivVectorSymbolizerItemPtr item =
	malloc (sizeof (rl2PrivVectorSymbolizerItem));
    rl2PrivTextSymbolizerPtr symbolizer =
	malloc (sizeof (rl2PrivTextSymbolizer));
    if (symbolizer == NULL || item == NULL)
      {
	  if (symbolizer != NULL)
	      free (symbolizer);
	  if (item != NULL)
	      free (item);
	  return NULL;
      }
    symbolizer->label = NULL;
    symbolizer->col_label = NULL;
    symbolizer->col_font = NULL;
    symbolizer->col_style = NULL;
    symbolizer->col_weight = NULL;
    symbolizer->col_size = NULL;
    symbolizer->font_families_count = 0;
    for (i = 0; i < RL2_MAX_FONT_FAMILIES; i++)
	*(symbolizer->font_families + i) = NULL;
    symbolizer->font_style = RL2_FONT_STYLE_NORMAL;
    symbolizer->font_weight = RL2_FONT_WEIGHT_NORMAL;
    symbolizer->font_size = 10.0;
    symbolizer->label_placement_type = RL2_LABEL_PLACEMENT_UNKNOWN;
    symbolizer->label_placement = NULL;
    symbolizer->halo = NULL;
    symbolizer->fill = NULL;
    item->symbolizer_type = RL2_TEXT_SYMBOLIZER;
    item->symbolizer = symbolizer;
    item->next = NULL;
    return item;
}

RL2_DECLARE int
rl2_is_valid_vector_symbolizer (rl2VectorSymbolizerPtr symbolizer, int *valid)
{
/* testing a Vector Symbolizer for validity */
    rl2PrivVectorSymbolizerPtr sym = (rl2PrivVectorSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->first == NULL)
	*valid = 0;
    else
	*valid = 1;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_vector_symbolizer_count (rl2VectorSymbolizerPtr symbolizer, int *count)
{
/* return the total count of Vector Symbolizer Items */
    int cnt = 0;
    rl2PrivVectorSymbolizerItemPtr item;
    rl2PrivVectorSymbolizerPtr sym = (rl2PrivVectorSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    item = sym->first;
    while (item != NULL)
      {
	  /* counting how many Items */
	  cnt++;
	  item = item->next;
      }
    *count = cnt;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_vector_symbolizer_item_type (rl2VectorSymbolizerPtr symbolizer,
				     int index, int *type)
{
/* return the Vector Symbolizer Item type */
    int cnt = 0;
    rl2PrivVectorSymbolizerItemPtr item;
    rl2PrivVectorSymbolizerPtr sym = (rl2PrivVectorSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    item = sym->first;
    while (item != NULL)
      {
	  if (cnt == index)
	    {
		*type = item->symbolizer_type;
		return RL2_OK;
	    }
	  cnt++;
	  item = item->next;
      }
    return RL2_ERROR;
}

RL2_DECLARE rl2PointSymbolizerPtr
rl2_get_point_symbolizer (rl2VectorSymbolizerPtr symbolizer, int index)
{
/* return a Point Symbolizer */
    int cnt = 0;
    rl2PrivVectorSymbolizerItemPtr item;
    rl2PrivVectorSymbolizerPtr sym = (rl2PrivVectorSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    item = sym->first;
    while (item != NULL)
      {
	  if (cnt == index)
	    {
		if (item->symbolizer_type == RL2_POINT_SYMBOLIZER)
		    return (rl2PointSymbolizerPtr) (item->symbolizer);
		else
		    return NULL;
	    }
	  cnt++;
	  item = item->next;
      }
    return NULL;
}

RL2_DECLARE rl2LineSymbolizerPtr
rl2_get_line_symbolizer (rl2VectorSymbolizerPtr symbolizer, int index)
{
/* return a Line Symbolizer */
    int cnt = 0;
    rl2PrivVectorSymbolizerItemPtr item;
    rl2PrivVectorSymbolizerPtr sym = (rl2PrivVectorSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    item = sym->first;
    while (item != NULL)
      {
	  if (cnt == index)
	    {
		if (item->symbolizer_type == RL2_LINE_SYMBOLIZER)
		    return (rl2LineSymbolizerPtr) (item->symbolizer);
		else
		    return NULL;
	    }
	  cnt++;
	  item = item->next;
      }
    return NULL;
}

RL2_DECLARE rl2PolygonSymbolizerPtr
rl2_get_polygon_symbolizer (rl2VectorSymbolizerPtr symbolizer, int index)
{
/* return a Polygon Symbolizer */
    int cnt = 0;
    rl2PrivVectorSymbolizerItemPtr item;
    rl2PrivVectorSymbolizerPtr sym = (rl2PrivVectorSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    item = sym->first;
    while (item != NULL)
      {
	  if (cnt == index)
	    {
		if (item->symbolizer_type == RL2_POLYGON_SYMBOLIZER)
		    return (rl2PolygonSymbolizerPtr) (item->symbolizer);
		else
		    return NULL;
	    }
	  cnt++;
	  item = item->next;
      }
    return NULL;
}

RL2_DECLARE rl2TextSymbolizerPtr
rl2_get_text_symbolizer (rl2VectorSymbolizerPtr symbolizer, int index)
{
/* return a Text Symbolizer */
    int cnt = 0;
    rl2PrivVectorSymbolizerItemPtr item;
    rl2PrivVectorSymbolizerPtr sym = (rl2PrivVectorSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    item = sym->first;
    while (item != NULL)
      {
	  if (cnt == index)
	    {
		if (item->symbolizer_type == RL2_TEXT_SYMBOLIZER)
		    return (rl2TextSymbolizerPtr) (item->symbolizer);
		else
		    return NULL;
	    }
	  cnt++;
	  item = item->next;
      }
    return NULL;
}

RL2_DECLARE int
rl2_line_symbolizer_has_stroke (rl2LineSymbolizerPtr symbolizer, int *stroke)
{
/* checks if a Line Symbolizer has a Stroke */
    rl2PrivLineSymbolizerPtr sym = (rl2PrivLineSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->stroke == NULL)
	*stroke = 0;
    else
	*stroke = 1;
    return RL2_OK;
}

RL2_DECLARE int
rl2_line_symbolizer_has_graphic_stroke (rl2LineSymbolizerPtr symbolizer,
					int *stroke)
{
/* checks if a Line Symbolizer has a Graphic Stroke */
    rl2PrivLineSymbolizerPtr sym = (rl2PrivLineSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    *stroke = 0;
    if (sym->stroke == NULL)
	;
    else
      {
	  if (sym->stroke->graphic != NULL)
	    {
		if (sym->stroke->graphic->first != NULL)
		  {
		      if (sym->stroke->graphic->first->type ==
			  RL2_EXTERNAL_GRAPHIC
			  && sym->stroke->graphic->first->item != NULL)
			  *stroke = 1;
		  }
	    }
      }
    return RL2_OK;
}

RL2_DECLARE const char *
rl2_line_symbolizer_get_graphic_stroke_href (rl2LineSymbolizerPtr symbolizer)
{
/* return an eventual Line Symbolizer Graphic Stroke xlink:href  */
    rl2PrivExternalGraphicPtr ext;
    rl2PrivLineSymbolizerPtr sym = (rl2PrivLineSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->stroke == NULL)
	return NULL;
    if (sym->stroke->graphic == NULL)
	return NULL;
    if (sym->stroke->graphic->first == NULL)
	return NULL;
    if (sym->stroke->graphic->first->type != RL2_EXTERNAL_GRAPHIC)
	return NULL;
    ext = (rl2PrivExternalGraphicPtr) (sym->stroke->graphic->first->item);
    return ext->xlink_href;
}

RL2_PRIVATE rl2PrivExternalGraphicPtr
rl2_line_symbolizer_get_stroke_external_graphic_ref (rl2LineSymbolizerPtr
						     symbolizer)
{
/* return a pointer to an External Graphic object (Line Stroke)  */
    rl2PrivExternalGraphicPtr ext;
    rl2PrivLineSymbolizerPtr sym = (rl2PrivLineSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->stroke == NULL)
	return NULL;
    if (sym->stroke->graphic == NULL)
	return NULL;
    if (sym->stroke->graphic->first == NULL)
	return NULL;
    if (sym->stroke->graphic->first->type != RL2_EXTERNAL_GRAPHIC)
	return NULL;
    ext = (rl2PrivExternalGraphicPtr) (sym->stroke->graphic->first->item);
    return ext;
}

RL2_DECLARE int
rl2_line_symbolizer_get_graphic_stroke_recode_count (rl2LineSymbolizerPtr
						     symbolizer, int *count)
{
/* return how many ColorReplacement items are in a Graphic Stroke (LineSymbolizer) */
    rl2PrivLineSymbolizerPtr sym = (rl2PrivLineSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    *count = 0;
    if (sym->stroke != NULL)
      {
	  if (sym->stroke->graphic != NULL)
	    {
		if (sym->stroke->graphic->first != NULL)
		  {
		      if (sym->stroke->graphic->first->type ==
			  RL2_EXTERNAL_GRAPHIC
			  && sym->stroke->graphic->first->item != NULL)
			{
			    int cnt = 0;
			    rl2PrivExternalGraphicPtr ext =
				(rl2PrivExternalGraphicPtr) (sym->
							     stroke->graphic->
							     first->item);
			    rl2PrivColorReplacementPtr repl = ext->first;
			    while (repl != NULL)
			      {
				  cnt++;
				  repl = repl->next;
			      }
			    *count = cnt;
			}
		  }
	    }
      }
    return RL2_OK;
}

RL2_DECLARE int
rl2_line_symbolizer_get_graphic_stroke_recode_color (rl2LineSymbolizerPtr
						     symbolizer, int index,
						     int *color_index,
						     unsigned char *red,
						     unsigned char *green,
						     unsigned char *blue)
{
/* return a ColorReplacement item from a Graphic Stroke (LineSymbolizer) */
    rl2PrivLineSymbolizerPtr sym = (rl2PrivLineSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->stroke != NULL)
      {
	  if (sym->stroke->graphic != NULL)
	    {
		if (sym->stroke->graphic->first != NULL)
		  {
		      if (sym->stroke->graphic->first->type ==
			  RL2_EXTERNAL_GRAPHIC
			  && sym->stroke->graphic->first->item != NULL)
			{
			    int cnt = 0;
			    rl2PrivExternalGraphicPtr ext =
				(rl2PrivExternalGraphicPtr) (sym->
							     stroke->graphic->
							     first->item);
			    rl2PrivColorReplacementPtr repl = ext->first;
			    while (repl != NULL)
			      {
				  if (cnt == index)
				    {
					*color_index = repl->index;
					*red = repl->red;
					*green = repl->green;
					*blue = repl->blue;
					return RL2_OK;
				    }
				  cnt++;
				  repl = repl->next;
			      }
			}
		  }
	    }
      }
    return RL2_ERROR;
}

RL2_PRIVATE rl2PrivColorReplacementPtr
rl2_line_symbolizer_get_stroke_color_replacement_ref (rl2LineSymbolizerPtr
						      symbolizer, int index,
						      int *color_index)
{
/* return a pointer to a ColorReplacement object (Line Stroke) */
    rl2PrivLineSymbolizerPtr sym = (rl2PrivLineSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->stroke != NULL)
      {
	  if (sym->stroke->graphic != NULL)
	    {
		if (sym->stroke->graphic->first != NULL)
		  {
		      if (sym->stroke->graphic->first->type ==
			  RL2_EXTERNAL_GRAPHIC
			  && sym->stroke->graphic->first->item != NULL)
			{
			    int cnt = 0;
			    rl2PrivExternalGraphicPtr ext =
				(rl2PrivExternalGraphicPtr) (sym->
							     stroke->graphic->
							     first->item);
			    rl2PrivColorReplacementPtr repl = ext->first;
			    while (repl != NULL)
			      {
				  if (cnt == index)
				    {
					*color_index = repl->index;
					return repl;
				    }
				  cnt++;
				  repl = repl->next;
			      }
			}
		  }
	    }
      }
    return NULL;
}

RL2_DECLARE const char *
rl2_line_symbolizer_get_col_graphic_stroke_href (rl2LineSymbolizerPtr
						 symbolizer)
{
/* return an eventual Line Symbolizer Table Column Name: Graphic Stroke Href */
    rl2PrivExternalGraphicPtr ext;
    rl2PrivLineSymbolizerPtr sym = (rl2PrivLineSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->stroke == NULL)
	return NULL;
    if (sym->stroke->graphic == NULL)
	return NULL;
    if (sym->stroke->graphic->first == NULL)
	return NULL;
    if (sym->stroke->graphic->first->type != RL2_EXTERNAL_GRAPHIC)
	return NULL;
    ext = (rl2PrivExternalGraphicPtr) (sym->stroke->graphic->first->item);
    return ext->col_href;
}

RL2_DECLARE const char *
rl2_line_symbolizer_get_col_graphic_stroke_recode_color (rl2LineSymbolizerPtr
							 symbolizer, int index,
							 int *color_index)
{
/* return an eventual Line Symbolizer Table Column Name: Graphic Stroke Color Replacement */
    rl2PrivLineSymbolizerPtr sym = (rl2PrivLineSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->stroke != NULL)
      {
	  if (sym->stroke->graphic != NULL)
	    {
		if (sym->stroke->graphic->first != NULL)
		  {
		      if (sym->stroke->graphic->first->type ==
			  RL2_EXTERNAL_GRAPHIC
			  && sym->stroke->graphic->first->item != NULL)
			{
			    int cnt = 0;
			    rl2PrivExternalGraphicPtr ext =
				(rl2PrivExternalGraphicPtr) (sym->
							     stroke->graphic->
							     first->item);
			    rl2PrivColorReplacementPtr repl = ext->first;
			    while (repl != NULL)
			      {
				  if (cnt == index)
				    {
					*color_index = repl->index;
					return repl->col_color;
				    }
				  cnt++;
				  repl = repl->next;
			      }
			}
		  }
	    }
      }
    return NULL;
}

RL2_DECLARE int
rl2_line_symbolizer_get_stroke_color (rl2LineSymbolizerPtr symbolizer,
				      unsigned char *red,
				      unsigned char *green, unsigned char *blue)
{
/* return the Line Symbolizer Stroke RGB color */
    rl2PrivLineSymbolizerPtr sym = (rl2PrivLineSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->stroke == NULL)
	return RL2_ERROR;
    *red = sym->stroke->red;
    *green = sym->stroke->green;
    *blue = sym->stroke->blue;
    return RL2_OK;
}

RL2_DECLARE int
rl2_line_symbolizer_get_stroke_opacity (rl2LineSymbolizerPtr symbolizer,
					double *opacity)
{
/* return the Line Symbolizer Stroke opacity */
    rl2PrivLineSymbolizerPtr sym = (rl2PrivLineSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->stroke == NULL)
	return RL2_ERROR;
    *opacity = sym->stroke->opacity;
    return RL2_OK;
}

RL2_DECLARE int
rl2_line_symbolizer_get_stroke_width (rl2LineSymbolizerPtr symbolizer,
				      double *width)
{
/* return the Line Symbolizer Stroke width */
    rl2PrivLineSymbolizerPtr sym = (rl2PrivLineSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->stroke == NULL)
	return RL2_ERROR;
    *width = sym->stroke->width;
    return RL2_OK;
}

RL2_DECLARE const char *
rl2_line_symbolizer_get_col_stroke_color (rl2LineSymbolizerPtr symbolizer)
{
/* return the Line Symbolizer Table Column Name: Stroke Color */
    rl2PrivLineSymbolizerPtr sym = (rl2PrivLineSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->stroke == NULL)
	return NULL;
    return sym->stroke->col_color;
}

RL2_DECLARE const char *
rl2_line_symbolizer_get_col_stroke_opacity (rl2LineSymbolizerPtr symbolizer)
{
/* return the Line Symbolizer Table Column Name: Stroke Opacity */
    rl2PrivLineSymbolizerPtr sym = (rl2PrivLineSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->stroke == NULL)
	return NULL;
    return sym->stroke->col_opacity;
}

RL2_DECLARE const char *
rl2_line_symbolizer_get_col_stroke_width (rl2LineSymbolizerPtr symbolizer)
{
/* return the Line Symbolizer Table Column Name: Stroke Width */
    rl2PrivLineSymbolizerPtr sym = (rl2PrivLineSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->stroke == NULL)
	return NULL;
    return sym->stroke->col_width;
}

RL2_DECLARE int
rl2_line_symbolizer_get_stroke_linejoin (rl2LineSymbolizerPtr symbolizer,
					 unsigned char *linejoin)
{
/* return the Line Symbolizer Stroke Linejoin mode */
    rl2PrivLineSymbolizerPtr sym = (rl2PrivLineSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->stroke == NULL)
	return RL2_ERROR;
    *linejoin = sym->stroke->linejoin;
    return RL2_OK;
}

RL2_DECLARE int
rl2_line_symbolizer_get_stroke_linecap (rl2LineSymbolizerPtr symbolizer,
					unsigned char *linecap)
{
/* return the Line Symbolizer Stroke Linecap mode */
    rl2PrivLineSymbolizerPtr sym = (rl2PrivLineSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->stroke == NULL)
	return RL2_ERROR;
    *linecap = sym->stroke->linecap;
    return RL2_OK;
}

RL2_DECLARE const char *
rl2_line_symbolizer_get_col_stroke_linejoin (rl2LineSymbolizerPtr symbolizer)
{
/* return the Line Symbolizer Table Column Name: Stroke LineJoin */
    rl2PrivLineSymbolizerPtr sym = (rl2PrivLineSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->stroke == NULL)
	return NULL;
    return sym->stroke->col_join;
}

RL2_DECLARE const char *
rl2_line_symbolizer_get_col_stroke_linecap (rl2LineSymbolizerPtr symbolizer)
{
/* return the Line Symbolizer Table Column Name: Stroke LineCap */
    rl2PrivLineSymbolizerPtr sym = (rl2PrivLineSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->stroke == NULL)
	return NULL;
    return sym->stroke->col_cap;
}

RL2_DECLARE int
rl2_line_symbolizer_get_stroke_dash_count (rl2LineSymbolizerPtr symbolizer,
					   int *count)
{
/* return the Line Symbolizer Stroke Dash count */
    rl2PrivLineSymbolizerPtr sym = (rl2PrivLineSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->stroke == NULL)
	return RL2_ERROR;
    *count = sym->stroke->dash_count;
    return RL2_OK;
}

RL2_DECLARE int
rl2_line_symbolizer_get_stroke_dash_item (rl2LineSymbolizerPtr symbolizer,
					  int index, double *item)
{
/* return a Line Symbolizer Stroke Dash item */
    rl2PrivLineSymbolizerPtr sym = (rl2PrivLineSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->stroke == NULL)
	return RL2_ERROR;
    if (sym->stroke->dash_list == NULL)
	return RL2_ERROR;
    if (index >= 0 && index < sym->stroke->dash_count)
	;
    else
	return RL2_ERROR;
    *item = *(sym->stroke->dash_list + index);
    return RL2_OK;
}

RL2_DECLARE int
rl2_line_symbolizer_get_stroke_dash_offset (rl2LineSymbolizerPtr symbolizer,
					    double *offset)
{
/* return the Line Symbolizer Stroke Dash initial offset */
    rl2PrivLineSymbolizerPtr sym = (rl2PrivLineSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->stroke == NULL)
	return RL2_ERROR;
    *offset = sym->stroke->dash_offset;
    return RL2_OK;
}

RL2_DECLARE const char *
rl2_line_symbolizer_get_col_stroke_dash_array (rl2LineSymbolizerPtr symbolizer)
{
/* return the Line Symbolizer Table Column Name: Stroke Dash Array */
    rl2PrivLineSymbolizerPtr sym = (rl2PrivLineSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->stroke == NULL)
	return NULL;
    return sym->stroke->col_dash;
}

RL2_DECLARE const char *
rl2_line_symbolizer_get_col_stroke_dash_offset (rl2LineSymbolizerPtr symbolizer)
{
/* return the Line Symbolizer Table Column Name: Stroke Dash Offset */
    rl2PrivLineSymbolizerPtr sym = (rl2PrivLineSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->stroke == NULL)
	return NULL;
    return sym->stroke->col_dashoff;
}

RL2_DECLARE int
rl2_line_symbolizer_get_perpendicular_offset (rl2LineSymbolizerPtr symbolizer,
					      double *offset)
{
/* return the Line Symbolizer perpendicular offset */
    rl2PrivLineSymbolizerPtr sym = (rl2PrivLineSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    *offset = sym->perpendicular_offset;
    return RL2_OK;
}

RL2_DECLARE const char *
rl2_line_symbolizer_get_col_perpendicular_offset (rl2LineSymbolizerPtr
						  symbolizer)
{
/* return the Line Symbolizer Table Column Name: Perpendicular Offset */
    rl2PrivLineSymbolizerPtr sym = (rl2PrivLineSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    return sym->col_perpoff;
}

RL2_DECLARE int
rl2_polygon_symbolizer_has_stroke (rl2PolygonSymbolizerPtr symbolizer,
				   int *stroke)
{
/* checks if a Polygon Symbolizer has a Stroke */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->stroke == NULL)
	*stroke = 0;
    else
	*stroke = 1;
    return RL2_OK;
}

RL2_DECLARE int
rl2_polygon_symbolizer_has_graphic_stroke (rl2PolygonSymbolizerPtr symbolizer,
					   int *stroke)
{
/* checks if a Polygon Symbolizer has a Graphic Stroke */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    *stroke = 0;
    if (sym->stroke == NULL)
	;
    else
      {
	  if (sym->stroke->graphic != NULL)
	    {
		if (sym->stroke->graphic->first != NULL)
		  {
		      if (sym->stroke->graphic->first->type ==
			  RL2_EXTERNAL_GRAPHIC
			  && sym->stroke->graphic->first->item != NULL)
			  *stroke = 1;
		  }
	    }
      }
    return RL2_OK;
}

RL2_PRIVATE rl2PrivColorReplacementPtr
rl2_polygon_symbolizer_get_stroke_color_replacement_ref (rl2PolygonSymbolizerPtr
							 symbolizer, int index,
							 int *color_index)
{
/* return a pointer to a ColorReplacement object (Polygon Stroke) */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->stroke != NULL)
      {
	  if (sym->stroke->graphic != NULL)
	    {
		if (sym->stroke->graphic->first != NULL)
		  {
		      if (sym->stroke->graphic->first->type ==
			  RL2_EXTERNAL_GRAPHIC
			  && sym->stroke->graphic->first->item != NULL)
			{
			    int cnt = 0;
			    rl2PrivExternalGraphicPtr ext =
				(rl2PrivExternalGraphicPtr) (sym->
							     stroke->graphic->
							     first->item);
			    rl2PrivColorReplacementPtr repl = ext->first;
			    while (repl != NULL)
			      {
				  if (cnt == index)
				    {
					*color_index = repl->index;
					return repl;
				    }
				  cnt++;
				  repl = repl->next;
			      }
			}
		  }
	    }
      }
    return NULL;
}

RL2_DECLARE const char *
rl2_polygon_symbolizer_get_graphic_stroke_href (rl2PolygonSymbolizerPtr
						symbolizer)
{
/* return an eventual Polygon Symbolizer Graphic Stroke xlink:href  */
    rl2PrivExternalGraphicPtr ext;
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->stroke == NULL)
	return NULL;
    if (sym->stroke->graphic == NULL)
	return NULL;
    if (sym->stroke->graphic->first == NULL)
	return NULL;
    if (sym->stroke->graphic->first->type != RL2_EXTERNAL_GRAPHIC)
	return NULL;
    ext = (rl2PrivExternalGraphicPtr) (sym->stroke->graphic->first->item);
    return ext->xlink_href;
}

RL2_PRIVATE rl2PrivExternalGraphicPtr
rl2_polygon_symbolizer_get_stroke_external_graphic_ref (rl2PolygonSymbolizerPtr
							symbolizer)
{
/* return a pointer to an External Graphic object (Polygon Stroke) */
    rl2PrivExternalGraphicPtr ext;
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->stroke == NULL)
	return NULL;
    if (sym->stroke->graphic == NULL)
	return NULL;
    if (sym->stroke->graphic->first == NULL)
	return NULL;
    if (sym->stroke->graphic->first->type != RL2_EXTERNAL_GRAPHIC)
	return NULL;
    ext = (rl2PrivExternalGraphicPtr) (sym->stroke->graphic->first->item);
    return ext;
}

RL2_DECLARE int
    rl2_polygon_symbolizer_get_graphic_stroke_recode_count
    (rl2PolygonSymbolizerPtr symbolizer, int *count)
{
/* return how many ColorReplacement items are in a Graphic Stroke (PolygonSymbolizer) */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    *count = 0;
    if (sym->stroke != NULL)
      {
	  if (sym->stroke->graphic != NULL)
	    {
		if (sym->stroke->graphic->first != NULL)
		  {
		      if (sym->stroke->graphic->first->type ==
			  RL2_EXTERNAL_GRAPHIC
			  && sym->stroke->graphic->first->item != NULL)
			{
			    int cnt = 0;
			    rl2PrivExternalGraphicPtr ext =
				(rl2PrivExternalGraphicPtr) (sym->
							     stroke->graphic->
							     first->item);
			    rl2PrivColorReplacementPtr repl = ext->first;
			    while (repl != NULL)
			      {
				  cnt++;
				  repl = repl->next;
			      }
			    *count = cnt;
			}
		  }
	    }
      }
    return RL2_OK;
}

RL2_DECLARE int
    rl2_polygon_symbolizer_get_graphic_stroke_recode_color
    (rl2PolygonSymbolizerPtr symbolizer, int index, int *color_index,
     unsigned char *red, unsigned char *green, unsigned char *blue)
{
/* return a ColorReplacement item from a Graphic Stroke (PolygonSymbolizer) */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->stroke != NULL)
      {
	  if (sym->stroke->graphic != NULL)
	    {
		if (sym->stroke->graphic->first != NULL)
		  {
		      if (sym->stroke->graphic->first->type ==
			  RL2_EXTERNAL_GRAPHIC
			  && sym->stroke->graphic->first->item != NULL)
			{
			    int cnt = 0;
			    rl2PrivExternalGraphicPtr ext =
				(rl2PrivExternalGraphicPtr) (sym->
							     stroke->graphic->
							     first->item);
			    rl2PrivColorReplacementPtr repl = ext->first;
			    while (repl != NULL)
			      {
				  if (cnt == index)
				    {
					*color_index = repl->index;
					*red = repl->red;
					*green = repl->green;
					*blue = repl->blue;
					return RL2_OK;
				    }
				  cnt++;
				  repl = repl->next;
			      }
			}
		  }
	    }
      }
    return RL2_ERROR;
}

RL2_DECLARE const char *
rl2_polygon_symbolizer_get_col_graphic_stroke_href (rl2PolygonSymbolizerPtr
						    symbolizer)
{
/* return a Polygon Symbolizer Table Column Name: Graphic Stroke Href  */
    rl2PrivExternalGraphicPtr ext;
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->stroke == NULL)
	return NULL;
    if (sym->stroke->graphic == NULL)
	return NULL;
    if (sym->stroke->graphic->first == NULL)
	return NULL;
    if (sym->stroke->graphic->first->type != RL2_EXTERNAL_GRAPHIC)
	return NULL;
    ext = (rl2PrivExternalGraphicPtr) (sym->stroke->graphic->first->item);
    return ext->col_href;
}

RL2_DECLARE const char
    *rl2_polygon_symbolizer_get_col_graphic_stroke_recode_color
    (rl2PolygonSymbolizerPtr symbolizer, int index, int *color_index)
{
/* return a Polygon Symbolizer Table Column Name: ColorReplacement */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->stroke != NULL)
      {
	  if (sym->stroke->graphic != NULL)
	    {
		if (sym->stroke->graphic->first != NULL)
		  {
		      if (sym->stroke->graphic->first->type ==
			  RL2_EXTERNAL_GRAPHIC
			  && sym->stroke->graphic->first->item != NULL)
			{
			    int cnt = 0;
			    rl2PrivExternalGraphicPtr ext =
				(rl2PrivExternalGraphicPtr) (sym->
							     stroke->graphic->
							     first->item);
			    rl2PrivColorReplacementPtr repl = ext->first;
			    while (repl != NULL)
			      {
				  if (cnt == index)
				    {
					*color_index = repl->index;
					return repl->col_color;
				    }
				  cnt++;
				  repl = repl->next;
			      }
			}
		  }
	    }
      }
    return NULL;
}

RL2_DECLARE int
rl2_polygon_symbolizer_get_stroke_color (rl2PolygonSymbolizerPtr symbolizer,
					 unsigned char *red,
					 unsigned char *green,
					 unsigned char *blue)
{
/* return the Polygon Symbolizer Stroke RGB color */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->stroke == NULL)
	return RL2_ERROR;
    *red = sym->stroke->red;
    *green = sym->stroke->green;
    *blue = sym->stroke->blue;
    return RL2_OK;
}

RL2_DECLARE int
rl2_polygon_symbolizer_get_stroke_opacity (rl2PolygonSymbolizerPtr symbolizer,
					   double *opacity)
{
/* return the Polygon Symbolizer Stroke opacity */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->stroke == NULL)
	return RL2_ERROR;
    *opacity = sym->stroke->opacity;
    return RL2_OK;
}

RL2_DECLARE int
rl2_polygon_symbolizer_get_stroke_width (rl2PolygonSymbolizerPtr symbolizer,
					 double *width)
{
/* return the Polygon Symbolizer Stroke width */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->stroke == NULL)
	return RL2_ERROR;
    *width = sym->stroke->width;
    return RL2_OK;
}

RL2_DECLARE const char *
rl2_polygon_symbolizer_get_col_stroke_color (rl2PolygonSymbolizerPtr symbolizer)
{
/* return the Polygon Symbolizer Stroke Table Column Name: Stroke Color */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->stroke == NULL)
	return NULL;
    return sym->stroke->col_color;
}

RL2_DECLARE const char *
rl2_polygon_symbolizer_get_col_stroke_opacity (rl2PolygonSymbolizerPtr
					       symbolizer)
{
/* return the Polygon Symbolizer Table Column Name: Stroke Opacity */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->stroke == NULL)
	return NULL;
    return sym->stroke->col_opacity;
}

RL2_DECLARE const char *
rl2_polygon_symbolizer_get_col_stroke_width (rl2PolygonSymbolizerPtr symbolizer)
{
/* return the Polygon Symbolizer Stroke width */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->stroke == NULL)
	return NULL;
    return sym->stroke->col_width;
}

RL2_DECLARE int
rl2_polygon_symbolizer_get_stroke_linejoin (rl2PolygonSymbolizerPtr
					    symbolizer, unsigned char *linejoin)
{
/* return the Polygon Symbolizer Stroke Linejoin mode */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->stroke == NULL)
	return RL2_ERROR;
    *linejoin = sym->stroke->linejoin;
    return RL2_OK;
}

RL2_DECLARE int
rl2_polygon_symbolizer_get_stroke_linecap (rl2PolygonSymbolizerPtr symbolizer,
					   unsigned char *linecap)
{
/* return the Polygon Symbolizer Stroke Linecap mode */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->stroke == NULL)
	return RL2_ERROR;
    *linecap = sym->stroke->linecap;
    return RL2_OK;
}

RL2_DECLARE const char *
rl2_polygon_symbolizer_get_col_stroke_linejoin (rl2PolygonSymbolizerPtr
						symbolizer)
{
/* return the Polygon Symbolizer Stroke LineJoin */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->stroke == NULL)
	return NULL;
    return sym->stroke->col_join;
}

RL2_DECLARE const char *
rl2_polygon_symbolizer_get_col_stroke_linecap (rl2PolygonSymbolizerPtr
					       symbolizer)
{
/* return the Polygon Symbolizer Stroke LineCap */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->stroke == NULL)
	return NULL;
    return sym->stroke->col_cap;
}

RL2_DECLARE int
rl2_polygon_symbolizer_get_stroke_dash_count (rl2PolygonSymbolizerPtr
					      symbolizer, int *count)
{
/* return the Polygon Symbolizer Stroke Dash count */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->stroke == NULL)
	return RL2_ERROR;
    *count = sym->stroke->dash_count;
    return RL2_OK;
}

RL2_DECLARE int
rl2_polygon_symbolizer_get_stroke_dash_item (rl2PolygonSymbolizerPtr
					     symbolizer, int index,
					     double *item)
{
/* return a Polygon Symbolizer Stroke Dash item */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->stroke == NULL)
	return RL2_ERROR;
    if (sym->stroke->dash_list == NULL)
	return RL2_ERROR;
    if (index >= 0 && index < sym->stroke->dash_count)
	;
    else
	return RL2_ERROR;
    *item = *(sym->stroke->dash_list + index);
    return RL2_OK;
}

RL2_DECLARE int
rl2_polygon_symbolizer_get_stroke_dash_offset (rl2PolygonSymbolizerPtr
					       symbolizer, double *offset)
{
/* return the Polygon Symbolizer Stroke Dash initial offset */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->stroke == NULL)
	return RL2_ERROR;
    *offset = sym->stroke->dash_offset;
    return RL2_OK;
}

RL2_DECLARE const char *
rl2_polygon_symbolizer_get_col_stroke_dash_array (rl2PolygonSymbolizerPtr
						  symbolizer)
{
/* return the Polygon Symbolizer Table Column Name: Stroke Dash Array */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->stroke == NULL)
	return NULL;
    return sym->stroke->col_dash;
}

RL2_DECLARE const char *
rl2_polygon_symbolizer_get_col_stroke_dash_offset (rl2PolygonSymbolizerPtr
						   symbolizer)
{
/* return the Polygon Symbolizer Table Column Name: Stroke Dash Offset */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->stroke == NULL)
	return NULL;
    return sym->stroke->col_dashoff;
}

RL2_DECLARE int
rl2_polygon_symbolizer_has_fill (rl2PolygonSymbolizerPtr symbolizer, int *fill)
{
/* checks if a Polygon Symbolizer has a Fill */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->fill == NULL)
	*fill = 0;
    else
	*fill = 1;
    return RL2_OK;
}

RL2_DECLARE int
rl2_polygon_symbolizer_has_graphic_fill (rl2PolygonSymbolizerPtr symbolizer,
					 int *fill)
{
/* checks if a Polygon Symbolizer has a Graphic Fill */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    *fill = 0;
    if (sym->fill == NULL)
	;
    else
      {
	  if (sym->fill->graphic != NULL)
	    {
		if (sym->fill->graphic->first != NULL)
		  {
		      if (sym->fill->graphic->first->type ==
			  RL2_EXTERNAL_GRAPHIC
			  && sym->fill->graphic->first->item != NULL)
			  *fill = 1;
		  }
	    }
      }
    return RL2_OK;
}

RL2_PRIVATE rl2PrivColorReplacementPtr
rl2_polygon_symbolizer_get_fill_color_replacement_ref (rl2PolygonSymbolizerPtr
						       symbolizer, int index,
						       int *color_index)
{
/* return a pointer to a ColorReplacement object (Polygon Fill) */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->fill != NULL)
      {
	  if (sym->fill->graphic != NULL)
	    {
		if (sym->fill->graphic->first != NULL)
		  {
		      if (sym->fill->graphic->first->type ==
			  RL2_EXTERNAL_GRAPHIC
			  && sym->fill->graphic->first->item != NULL)
			{
			    int cnt = 0;
			    rl2PrivExternalGraphicPtr ext =
				(rl2PrivExternalGraphicPtr) (sym->
							     fill->graphic->
							     first->item);
			    rl2PrivColorReplacementPtr repl = ext->first;
			    while (repl != NULL)
			      {
				  if (cnt == index)
				    {
					*color_index = repl->index;
					return repl;
				    }
				  cnt++;
				  repl = repl->next;
			      }
			}
		  }
	    }
      }
    return NULL;
}

RL2_DECLARE const char *
rl2_polygon_symbolizer_get_graphic_fill_href (rl2PolygonSymbolizerPtr
					      symbolizer)
{
/* return an eventual Polygon Symbolizer Graphic Fill xlink:href  */
    rl2PrivExternalGraphicPtr ext;
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->fill == NULL)
	return NULL;
    if (sym->fill->graphic == NULL)
	return NULL;
    if (sym->fill->graphic->first == NULL)
	return NULL;
    if (sym->fill->graphic->first->type != RL2_EXTERNAL_GRAPHIC)
	return NULL;
    ext = (rl2PrivExternalGraphicPtr) (sym->fill->graphic->first->item);
    return ext->xlink_href;
}

RL2_PRIVATE rl2PrivExternalGraphicPtr
rl2_polygon_symbolizer_get_fill_external_graphic_ref (rl2PolygonSymbolizerPtr
						      symbolizer)
{
/* return a pointer to an External Graphic object (Polygon Fill) */
    rl2PrivExternalGraphicPtr ext;
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->fill == NULL)
	return NULL;
    if (sym->fill->graphic == NULL)
	return NULL;
    if (sym->fill->graphic->first == NULL)
	return NULL;
    if (sym->fill->graphic->first->type != RL2_EXTERNAL_GRAPHIC)
	return NULL;
    ext = (rl2PrivExternalGraphicPtr) (sym->fill->graphic->first->item);
    return ext;
}

RL2_DECLARE int
rl2_polygon_symbolizer_get_graphic_fill_recode_count (rl2PolygonSymbolizerPtr
						      symbolizer, int *count)
{
/* return how many ColorReplacement items are in a Graphic Fill (PolygonSymbolizer) */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    *count = 0;
    if (sym->fill != NULL)
      {
	  if (sym->fill->graphic != NULL)
	    {
		if (sym->fill->graphic->first != NULL)
		  {
		      if (sym->fill->graphic->first->type ==
			  RL2_EXTERNAL_GRAPHIC
			  && sym->fill->graphic->first->item != NULL)
			{
			    int cnt = 0;
			    rl2PrivExternalGraphicPtr ext =
				(rl2PrivExternalGraphicPtr) (sym->
							     fill->graphic->
							     first->item);
			    rl2PrivColorReplacementPtr repl = ext->first;
			    while (repl != NULL)
			      {
				  cnt++;
				  repl = repl->next;
			      }
			    *count = cnt;
			}
		  }
	    }
      }
    return RL2_OK;
}

RL2_DECLARE int
rl2_polygon_symbolizer_get_graphic_fill_recode_color (rl2PolygonSymbolizerPtr
						      symbolizer, int index,
						      int *color_index,
						      unsigned char *red,
						      unsigned char *green,
						      unsigned char *blue)
{
/* return a ColorReplacement item from a Graphic Fill (PolygonSymbolizer) */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->fill != NULL)
      {
	  if (sym->fill->graphic != NULL)
	    {
		if (sym->fill->graphic->first != NULL)
		  {
		      if (sym->fill->graphic->first->type ==
			  RL2_EXTERNAL_GRAPHIC
			  && sym->fill->graphic->first->item != NULL)
			{
			    int cnt = 0;
			    rl2PrivExternalGraphicPtr ext =
				(rl2PrivExternalGraphicPtr) (sym->
							     fill->graphic->
							     first->item);
			    rl2PrivColorReplacementPtr repl = ext->first;
			    while (repl != NULL)
			      {
				  if (cnt == index)
				    {
					*color_index = repl->index;
					*red = repl->red;
					*green = repl->green;
					*blue = repl->blue;
					return RL2_OK;
				    }
				  cnt++;
				  repl = repl->next;
			      }
			}
		  }
	    }
      }
    return RL2_ERROR;
}

RL2_DECLARE const char *
rl2_polygon_symbolizer_get_col_graphic_fill_href (rl2PolygonSymbolizerPtr
						  symbolizer)
{
/* Polygon Symbolizer Table Column Name: Graphic Fill Href  */
    rl2PrivExternalGraphicPtr ext;
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->fill == NULL)
	return NULL;
    if (sym->fill->graphic == NULL)
	return NULL;
    if (sym->fill->graphic->first == NULL)
	return NULL;
    if (sym->fill->graphic->first->type != RL2_EXTERNAL_GRAPHIC)
	return NULL;
    ext = (rl2PrivExternalGraphicPtr) (sym->fill->graphic->first->item);
    return ext->col_href;
}

RL2_DECLARE const char *rl2_polygon_symbolizer_get_col_graphic_fill_recode_color
    (rl2PolygonSymbolizerPtr symbolizer, int index, int *color_index)
{
/* Polygon Symbolizer Table Column Name: Graphic Fill Colort Replacement */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->fill != NULL)
      {
	  if (sym->fill->graphic != NULL)
	    {
		if (sym->fill->graphic->first != NULL)
		  {
		      if (sym->fill->graphic->first->type ==
			  RL2_EXTERNAL_GRAPHIC
			  && sym->fill->graphic->first->item != NULL)
			{
			    int cnt = 0;
			    rl2PrivExternalGraphicPtr ext =
				(rl2PrivExternalGraphicPtr) (sym->
							     fill->graphic->
							     first->item);
			    rl2PrivColorReplacementPtr repl = ext->first;
			    while (repl != NULL)
			      {
				  if (cnt == index)
				    {
					*color_index = repl->index;
					return repl->col_color;
				    }
				  cnt++;
				  repl = repl->next;
			      }
			}
		  }
	    }
      }
    return NULL;
}

RL2_DECLARE int
rl2_polygon_symbolizer_get_fill_color (rl2PolygonSymbolizerPtr symbolizer,
				       unsigned char *red,
				       unsigned char *green,
				       unsigned char *blue)
{
/* return the Polygon Symbolizer Fill RGB color */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->fill == NULL)
	return RL2_ERROR;
    *red = sym->fill->red;
    *green = sym->fill->green;
    *blue = sym->fill->blue;
    return RL2_OK;
}

RL2_DECLARE int
rl2_polygon_symbolizer_get_fill_opacity (rl2PolygonSymbolizerPtr symbolizer,
					 double *opacity)
{
/* return the Polygon Symbolizer Fill opacity */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->fill == NULL)
	return RL2_ERROR;
    *opacity = sym->fill->opacity;
    return RL2_OK;
}

RL2_DECLARE int
rl2_polygon_symbolizer_get_perpendicular_offset (rl2PolygonSymbolizerPtr
						 symbolizer, double *offset)
{
/* return the Polygon Symbolizer perpendicular offset */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    *offset = sym->perpendicular_offset;
    return RL2_OK;
}

RL2_DECLARE int
rl2_polygon_symbolizer_get_displacement (rl2PolygonSymbolizerPtr symbolizer,
					 double *x, double *y)
{
/* return the Polygon Symbolizer Displacement */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    *x = sym->displacement_x;
    *y = sym->displacement_y;
    return RL2_OK;
}

RL2_DECLARE const char *
rl2_polygon_symbolizer_get_col_fill_color (rl2PolygonSymbolizerPtr symbolizer)
{
/* return the Polygon Symbolizer Table Column Name: Fill Color */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->fill == NULL)
	return NULL;
    return sym->fill->col_color;
}

RL2_DECLARE const char *
rl2_polygon_symbolizer_get_col_fill_opacity (rl2PolygonSymbolizerPtr symbolizer)
{
/* return the Polygon Symbolizer Table Column Name: Fill opacity */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->fill == NULL)
	return NULL;
    return sym->fill->col_opacity;
}

RL2_DECLARE const char *
rl2_polygon_symbolizer_get_col_perpendicular_offset (rl2PolygonSymbolizerPtr
						     symbolizer)
{
/* return the Polygon Symbolizer Table Column Name: perpendicular offset */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    return sym->col_perpoff;
}

RL2_DECLARE const char *
rl2_polygon_symbolizer_get_col_displacement_x (rl2PolygonSymbolizerPtr
					       symbolizer)
{
/* return the Polygon Symbolizer Table Column Name: displacement X */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    return sym->col_displ_x;
}

RL2_DECLARE const char *
rl2_polygon_symbolizer_get_col_displacement_y (rl2PolygonSymbolizerPtr
					       symbolizer)
{
/* return the Polygon Symbolizer Table Column Name: displacement Y */
    rl2PrivPolygonSymbolizerPtr sym = (rl2PrivPolygonSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    return sym->col_displ_y;
}

RL2_DECLARE const char *
rl2_text_symbolizer_get_label (rl2TextSymbolizerPtr symbolizer)
{
/* return the Text Symbolizer label name */
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    return sym->label;
}

RL2_DECLARE const char *
rl2_text_symbolizer_get_col_label (rl2TextSymbolizerPtr symbolizer)
{
/* return the Text Symbolizer table column name: label */
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    return sym->col_label;
}

RL2_DECLARE const char *
rl2_text_symbolizer_get_col_font (rl2TextSymbolizerPtr symbolizer)
{
/* return the Text Symbolizer table column name: font facename */
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    return sym->col_font;
}

RL2_DECLARE const char *
rl2_text_symbolizer_get_col_style (rl2TextSymbolizerPtr symbolizer)
{
/* return the Text Symbolizer table column name: font style */
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    return sym->col_style;
}

RL2_DECLARE const char *
rl2_text_symbolizer_get_col_weight (rl2TextSymbolizerPtr symbolizer)
{
/* return the Text Symbolizer table column name: font weight */
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    return sym->col_weight;
}

RL2_DECLARE const char *
rl2_text_symbolizer_get_col_size (rl2TextSymbolizerPtr symbolizer)
{
/* return the Text Symbolizer table column name: font size */
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    return sym->col_size;
}

RL2_DECLARE int
rl2_text_symbolizer_get_font_families_count (rl2TextSymbolizerPtr symbolizer,
					     int *count)
{
/* return how many Font Families are defined (Text Symbolizer) */
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    *count = sym->font_families_count;
    return RL2_OK;
}

RL2_DECLARE const char *
rl2_text_symbolizer_get_font_family_name (rl2TextSymbolizerPtr symbolizer,
					  int index)
{
/* return the Nth Text Symbolizer FontFamily name */
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (index >= 0 && index < sym->font_families_count)
	return *(sym->font_families + index);
    return NULL;
}

RL2_DECLARE int
rl2_text_symbolizer_get_font_style (rl2TextSymbolizerPtr symbolizer,
				    unsigned char *style)
{
/* return the Text Symbolizer Font Style */
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->font_style == RL2_FONT_STYLE_ITALIC
	|| sym->font_style == RL2_FONT_STYLE_OBLIQUE)
	*style = sym->font_style;
    else
	*style = RL2_FONT_STYLE_NORMAL;
    return RL2_OK;
}

RL2_DECLARE int
rl2_text_symbolizer_get_font_weight (rl2TextSymbolizerPtr symbolizer,
				     unsigned char *weight)
{
/* return the Text Symbolizer Font Weight */
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->font_weight == RL2_FONT_WEIGHT_BOLD)
	*weight = sym->font_weight;
    else
	*weight = RL2_FONT_WEIGHT_NORMAL;
    return RL2_OK;
}

RL2_DECLARE int
rl2_text_symbolizer_get_font_size (rl2TextSymbolizerPtr symbolizer,
				   double *size)
{
/* return the Text Symbolizer Font Size */
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    *size = sym->font_size;
    return RL2_OK;
}

RL2_DECLARE int
rl2_text_symbolizer_get_label_placement_mode (rl2TextSymbolizerPtr symbolizer,
					      unsigned char *mode)
{
/* return the Text Symbolizer LabelPlacement mode */
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->label_placement == NULL)
	*mode = RL2_LABEL_PLACEMENT_UNKNOWN;
    else
      {
	  if (sym->label_placement_type == RL2_LABEL_PLACEMENT_POINT
	      || sym->label_placement_type == RL2_LABEL_PLACEMENT_LINE)
	      *mode = sym->label_placement_type;
	  else
	      *mode = RL2_LABEL_PLACEMENT_UNKNOWN;
      }
    return RL2_OK;
}

RL2_DECLARE int
rl2_text_symbolizer_get_point_placement_anchor_point (rl2TextSymbolizerPtr
						      symbolizer, double *x,
						      double *y)
{
/* return the Text Symbolizer PointPlacement AnchorPoint */
    rl2PrivPointPlacementPtr place;
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->label_placement_type != RL2_LABEL_PLACEMENT_POINT
	|| sym->label_placement == NULL)
	return RL2_ERROR;
    place = (rl2PrivPointPlacementPtr) (sym->label_placement);
    *x = place->anchor_point_x;
    *y = place->anchor_point_y;
    return RL2_OK;
}

RL2_DECLARE int
rl2_text_symbolizer_get_point_placement_displacement (rl2TextSymbolizerPtr
						      symbolizer, double *x,
						      double *y)
{
/* return the Text Symbolizer PointPlacement Displacement */
    rl2PrivPointPlacementPtr place;
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->label_placement_type != RL2_LABEL_PLACEMENT_POINT
	|| sym->label_placement == NULL)
	return RL2_ERROR;
    place = (rl2PrivPointPlacementPtr) (sym->label_placement);
    *x = place->displacement_x;
    *y = place->displacement_y;
    return RL2_OK;
}

RL2_DECLARE int
rl2_text_symbolizer_get_point_placement_rotation (rl2TextSymbolizerPtr
						  symbolizer, double *rotation)
{
/* return the Text Symbolizer PointPlacement Rotation */
    rl2PrivPointPlacementPtr place;
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->label_placement_type != RL2_LABEL_PLACEMENT_POINT
	|| sym->label_placement == NULL)
	return RL2_ERROR;
    place = (rl2PrivPointPlacementPtr) (sym->label_placement);
    *rotation = place->rotation;
    return RL2_OK;
}

RL2_DECLARE const char *
rl2_text_symbolizer_get_point_placement_col_anchor_point_x (rl2TextSymbolizerPtr
							    symbolizer)
{
/* return the Text Symbolizer PointPlacement table column name: Anchor Point X */
    rl2PrivPointPlacementPtr place;
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->label_placement_type != RL2_LABEL_PLACEMENT_POINT
	|| sym->label_placement == NULL)
	return NULL;
    place = (rl2PrivPointPlacementPtr) (sym->label_placement);
    return place->col_point_x;
}

RL2_DECLARE const char *
rl2_text_symbolizer_get_point_placement_col_anchor_point_y (rl2TextSymbolizerPtr
							    symbolizer)
{
/* return the Text Symbolizer PointPlacement table column name: Anchor Point Y */
    rl2PrivPointPlacementPtr place;
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->label_placement_type != RL2_LABEL_PLACEMENT_POINT
	|| sym->label_placement == NULL)
	return NULL;
    place = (rl2PrivPointPlacementPtr) (sym->label_placement);
    return place->col_point_y;
}

RL2_DECLARE const char *
rl2_text_symbolizer_get_point_placement_col_displacement_x (rl2TextSymbolizerPtr
							    symbolizer)
{
/* return the Text Symbolizer PointPlacement table column name: Displacement X */
    rl2PrivPointPlacementPtr place;
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->label_placement_type != RL2_LABEL_PLACEMENT_POINT
	|| sym->label_placement == NULL)
	return NULL;
    place = (rl2PrivPointPlacementPtr) (sym->label_placement);
    return place->col_displ_x;
}

RL2_DECLARE const char *
rl2_text_symbolizer_get_point_placement_col_displacement_y (rl2TextSymbolizerPtr
							    symbolizer)
{
/* return the Text Symbolizer PointPlacement table column name: Displacement Y */
    rl2PrivPointPlacementPtr place;
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->label_placement_type != RL2_LABEL_PLACEMENT_POINT
	|| sym->label_placement == NULL)
	return NULL;
    place = (rl2PrivPointPlacementPtr) (sym->label_placement);
    return place->col_displ_y;
}

RL2_DECLARE const char *
rl2_text_symbolizer_get_point_placement_col_rotation (rl2TextSymbolizerPtr
						      symbolizer)
{
/* return the Text Symbolizer PointPlacement table column name: Rotation  */
    rl2PrivPointPlacementPtr place;
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->label_placement_type != RL2_LABEL_PLACEMENT_POINT
	|| sym->label_placement == NULL)
	return NULL;
    place = (rl2PrivPointPlacementPtr) (sym->label_placement);
    return place->col_rotation;
}

RL2_DECLARE int
    rl2_text_symbolizer_get_line_placement_perpendicular_offset
    (rl2TextSymbolizerPtr symbolizer, double *offset)
{
/* return the Text Symbolizer LinePlacement PerpendicularOffset */
    rl2PrivLinePlacementPtr place;
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->label_placement_type != RL2_LABEL_PLACEMENT_LINE
	|| sym->label_placement == NULL)
	return RL2_ERROR;
    place = (rl2PrivLinePlacementPtr) (sym->label_placement);
    *offset = place->perpendicular_offset;
    return RL2_OK;
}

RL2_DECLARE int
rl2_text_symbolizer_get_line_placement_is_repeated (rl2TextSymbolizerPtr
						    symbolizer,
						    int *is_repeated)
{
/* return the Text Symbolizer LinePlacement IsRepeated */
    rl2PrivLinePlacementPtr place;
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->label_placement_type != RL2_LABEL_PLACEMENT_LINE
	|| sym->label_placement == NULL)
	return RL2_ERROR;
    place = (rl2PrivLinePlacementPtr) (sym->label_placement);
    *is_repeated = place->is_repeated;
    return RL2_OK;
}

RL2_DECLARE int
rl2_text_symbolizer_get_line_placement_initial_gap (rl2TextSymbolizerPtr
						    symbolizer, double *gap)
{
/* return the Text Symbolizer LinePlacement InitialGap */
    rl2PrivLinePlacementPtr place;
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->label_placement_type != RL2_LABEL_PLACEMENT_LINE
	|| sym->label_placement == NULL)
	return RL2_ERROR;
    place = (rl2PrivLinePlacementPtr) (sym->label_placement);
    *gap = place->initial_gap;
    return RL2_OK;
}

RL2_DECLARE int
rl2_text_symbolizer_get_line_placement_gap (rl2TextSymbolizerPtr symbolizer,
					    double *gap)
{
/* return the Text Symbolizer LinePlacement Gap */
    rl2PrivLinePlacementPtr place;
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->label_placement_type != RL2_LABEL_PLACEMENT_LINE
	|| sym->label_placement == NULL)
	return RL2_ERROR;
    place = (rl2PrivLinePlacementPtr) (sym->label_placement);
    *gap = place->gap;
    return RL2_OK;
}

RL2_DECLARE int
rl2_text_symbolizer_get_line_placement_is_aligned (rl2TextSymbolizerPtr
						   symbolizer, int *is_aligned)
{
/* return the Text Symbolizer LinePlacement IsAligned */
    rl2PrivLinePlacementPtr place;
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->label_placement_type != RL2_LABEL_PLACEMENT_LINE
	|| sym->label_placement == NULL)
	return RL2_ERROR;
    place = (rl2PrivLinePlacementPtr) (sym->label_placement);
    *is_aligned = place->is_aligned;
    return RL2_OK;
}

RL2_DECLARE int
rl2_text_symbolizer_get_line_placement_generalize_line (rl2TextSymbolizerPtr
							symbolizer,
							int *generalize_line)
{
/* return the Text Symbolizer LinePlacement GeneralizeLine */
    rl2PrivLinePlacementPtr place;
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->label_placement_type != RL2_LABEL_PLACEMENT_LINE
	|| sym->label_placement == NULL)
	return RL2_ERROR;
    place = (rl2PrivLinePlacementPtr) (sym->label_placement);
    *generalize_line = place->generalize_line;
    return RL2_OK;
}

RL2_DECLARE const char
    *rl2_text_symbolizer_get_line_placement_col_perpendicular_offset
    (rl2TextSymbolizerPtr symbolizer)
{
/* return the Text Symbolizer LinePlacement Column Name: PerpendicularOffset */
    rl2PrivLinePlacementPtr place;
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->label_placement_type != RL2_LABEL_PLACEMENT_LINE
	|| sym->label_placement == NULL)
	return NULL;
    place = (rl2PrivLinePlacementPtr) (sym->label_placement);
    return place->col_perpoff;
}

RL2_DECLARE const char *
rl2_text_symbolizer_get_line_placement_col_initial_gap (rl2TextSymbolizerPtr
							symbolizer)
{
/* return the Text Symbolizer LinePlacement Column Name: InitialGap */
    rl2PrivLinePlacementPtr place;
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->label_placement_type != RL2_LABEL_PLACEMENT_LINE
	|| sym->label_placement == NULL)
	return NULL;
    place = (rl2PrivLinePlacementPtr) (sym->label_placement);
    return place->col_inigap;
}

RL2_DECLARE const char *
rl2_text_symbolizer_get_line_placement_col_gap (rl2TextSymbolizerPtr symbolizer)
{
/* return the Text Symbolizer LinePlacement Column Name: Gap */
    rl2PrivLinePlacementPtr place;
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->label_placement_type != RL2_LABEL_PLACEMENT_LINE
	|| sym->label_placement == NULL)
	return NULL;
    place = (rl2PrivLinePlacementPtr) (sym->label_placement);
    return place->col_gap;
}

RL2_DECLARE int
rl2_text_symbolizer_has_halo (rl2TextSymbolizerPtr symbolizer, int *halo)
{
/* checks if a Text Symbolizer has an Halo */
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->halo == NULL)
	*halo = 0;
    else
	*halo = 1;
    return RL2_OK;
}

RL2_DECLARE int
rl2_text_symbolizer_get_halo_radius (rl2TextSymbolizerPtr symbolizer,
				     double *radius)
{
/* return the Text Symbolizer Halo Radius */
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->halo == NULL)
	return RL2_ERROR;
    *radius = sym->halo->radius;
    return RL2_OK;
}

RL2_DECLARE const char *
rl2_text_symbolizer_get_halo_col_radius (rl2TextSymbolizerPtr symbolizer)
{
/* return the Text Symbolizer Halo Table Column Name: Radius */
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->halo == NULL)
	return NULL;
    return sym->halo->col_radius;
}

RL2_DECLARE int
rl2_text_symbolizer_has_halo_fill (rl2TextSymbolizerPtr symbolizer, int *fill)
{
/* checks if a Text Symbolizer Halo has a Fill */
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->halo == NULL)
	return RL2_ERROR;
    if (sym->halo->fill == NULL)
	*fill = 0;
    else
	*fill = 1;
    return RL2_OK;
}

RL2_DECLARE int
rl2_text_symbolizer_get_halo_fill_color (rl2TextSymbolizerPtr symbolizer,
					 unsigned char *red,
					 unsigned char *green,
					 unsigned char *blue)
{
/* return the Text Symbolizer Halo Fill RGB color */
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->halo == NULL)
	return RL2_ERROR;
    if (sym->halo->fill == NULL)
	return RL2_ERROR;
    *red = sym->halo->fill->red;
    *green = sym->halo->fill->green;
    *blue = sym->halo->fill->blue;
    return RL2_OK;
}

RL2_DECLARE const char *
rl2_text_symbolizer_get_halo_col_fill_color (rl2TextSymbolizerPtr symbolizer)
{
/* return the Text Symbolizer Halo Table Column Name: Fill Colour */
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->halo == NULL)
	return NULL;
    if (sym->halo->fill == NULL)
	return NULL;
    return sym->halo->fill->col_color;
}

RL2_DECLARE int
rl2_text_symbolizer_get_halo_fill_opacity (rl2TextSymbolizerPtr symbolizer,
					   double *opacity)
{
/* return the Text Symbolizer Halo Opacity */
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->halo == NULL)
	return RL2_ERROR;
    if (sym->halo->fill == NULL)
	return RL2_ERROR;
    *opacity = sym->halo->fill->opacity;
    return RL2_OK;
}

RL2_DECLARE const char *
rl2_text_symbolizer_get_halo_col_fill_opacity (rl2TextSymbolizerPtr symbolizer)
{
/* return the Text Symbolizer Table Column Name: Halo Opacity */
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->halo == NULL)
	return NULL;
    if (sym->halo->fill == NULL)
	return NULL;
    return sym->halo->fill->col_opacity;
}

RL2_DECLARE int
rl2_text_symbolizer_has_fill (rl2TextSymbolizerPtr symbolizer, int *fill)
{
/* checks if a Text Symbolizer has a Fill */
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->fill == NULL)
	*fill = 0;
    else
	*fill = 1;
    return RL2_OK;
}

RL2_DECLARE int
rl2_text_symbolizer_get_fill_color (rl2TextSymbolizerPtr symbolizer,
				    unsigned char *red,
				    unsigned char *green, unsigned char *blue)
{
/* return the Text Symbolizer Fill RGB color */
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->fill == NULL)
	return RL2_ERROR;
    *red = sym->fill->red;
    *green = sym->fill->green;
    *blue = sym->fill->blue;
    return RL2_OK;
}

RL2_DECLARE const char *
rl2_text_symbolizer_get_col_fill_color (rl2TextSymbolizerPtr symbolizer)
{
/* return the Text Symbolizer Table Column Name: Fill Color */
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->fill == NULL)
	return NULL;
    return sym->fill->col_color;
}

RL2_DECLARE int
rl2_text_symbolizer_get_fill_opacity (rl2TextSymbolizerPtr symbolizer,
				      double *opacity)
{
/* return the Text Symbolizer Fill Opacity */
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->fill == NULL)
	return RL2_ERROR;
    *opacity = sym->fill->opacity;
    return RL2_OK;
}

RL2_DECLARE const char *
rl2_text_symbolizer_get_col_fill_opacity (rl2TextSymbolizerPtr symbolizer)
{
/* return the Text Symbolizer Table Column Name: Fill Opacity */
    rl2PrivTextSymbolizerPtr sym = (rl2PrivTextSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->fill == NULL)
	return NULL;
    return sym->fill->col_opacity;
}

RL2_DECLARE int
rl2_point_symbolizer_get_count (rl2PointSymbolizerPtr symbolizer, int *count)
{
/* return the Point Symbolizer items count */
    int cnt = 0;
    rl2PrivGraphicItemPtr item;
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->graphic == NULL)
	return RL2_ERROR;
    item = sym->graphic->first;
    while (item != NULL)
      {
	  cnt++;
	  item = item->next;
      }
    *count = cnt;
    return RL2_OK;
}

RL2_DECLARE int
rl2_point_symbolizer_is_graphic (rl2PointSymbolizerPtr symbolizer, int index,
				 int *external_graphic)
{
/* checks if a Point Symbolizer item is an External Graphic */
    int count = 0;
    rl2PrivGraphicItemPtr item;
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->graphic == NULL)
	return RL2_ERROR;
    item = sym->graphic->first;
    while (item != NULL)
      {
	  if (count == index)
	    {
		if (item->type == RL2_EXTERNAL_GRAPHIC && item->item != NULL)
		    *external_graphic = 1;
		else
		    *external_graphic = 0;
		return RL2_OK;
	    }
	  count++;
	  item = item->next;
      }
    return RL2_ERROR;
}

RL2_DECLARE const char *
rl2_point_symbolizer_get_graphic_href (rl2PointSymbolizerPtr symbolizer,
				       int index)
{
/* return the Point Symbolizer External Graphic xlink:href */
    int count = 0;
    rl2PrivExternalGraphicPtr ext;
    rl2PrivGraphicItemPtr item;
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->graphic == NULL)
	return NULL;
    item = sym->graphic->first;
    while (item != NULL)
      {
	  if (count == index)
	    {
		if (item->type == RL2_EXTERNAL_GRAPHIC && item->item != NULL)
		    ext = (rl2PrivExternalGraphicPtr) (item->item);
		else
		    return NULL;
		return ext->xlink_href;
	    }
	  count++;
	  item = item->next;
      }
    return NULL;
}

RL2_DECLARE int
rl2_point_symbolizer_get_graphic_recode_color (rl2PointSymbolizerPtr
					       symbolizer, int index,
					       int repl_index,
					       int *color_index,
					       unsigned char *red,
					       unsigned char *green,
					       unsigned char *blue)
{
/* return a ColorReplacement item from an External Graphic (PointSymbolizer) */
    int count = 0;
    rl2PrivExternalGraphicPtr ext;
    rl2PrivGraphicItemPtr item;
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->graphic == NULL)
	return RL2_ERROR;
    item = sym->graphic->first;
    while (item != NULL)
      {
	  if (count == index)
	    {
		int cnt = 0;
		rl2PrivColorReplacementPtr repl;
		if (item->type == RL2_EXTERNAL_GRAPHIC && item->item != NULL)
		    ext = (rl2PrivExternalGraphicPtr) (item->item);
		else
		    return RL2_ERROR;
		repl = ext->first;
		while (repl != NULL)
		  {
		      if (cnt == repl_index)
			{
			    *color_index = repl->index;
			    *red = repl->red;
			    *green = repl->green;
			    *blue = repl->blue;
			    return RL2_OK;
			}
		      cnt++;
		      repl = repl->next;
		  }
		return RL2_ERROR;
	    }
	  count++;
	  item = item->next;
      }
    return RL2_ERROR;
}

RL2_DECLARE const char *
rl2_point_symbolizer_get_col_graphic_href (rl2PointSymbolizerPtr symbolizer,
					   int index)
{
/* return the Point Symbolizer Table Column Name: External Graphic Href */
    int count = 0;
    rl2PrivExternalGraphicPtr ext;
    rl2PrivGraphicItemPtr item;
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->graphic == NULL)
	return NULL;
    item = sym->graphic->first;
    while (item != NULL)
      {
	  if (count == index)
	    {
		if (item->type == RL2_EXTERNAL_GRAPHIC && item->item != NULL)
		    ext = (rl2PrivExternalGraphicPtr) (item->item);
		else
		    return NULL;
		return ext->col_href;
	    }
	  count++;
	  item = item->next;
      }
    return NULL;
}

RL2_PRIVATE rl2PrivExternalGraphicPtr
rl2_point_symbolizer_get_external_graphic_ref (rl2PointSymbolizerPtr symbolizer,
					       int index)
{
/* return a pointer to an External Graphic object */
    int count = 0;
    rl2PrivExternalGraphicPtr ext;
    rl2PrivGraphicItemPtr item;
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->graphic == NULL)
	return NULL;
    item = sym->graphic->first;
    while (item != NULL)
      {
	  if (count == index)
	    {
		if (item->type == RL2_EXTERNAL_GRAPHIC && item->item != NULL)
		    ext = (rl2PrivExternalGraphicPtr) (item->item);
		else
		    return NULL;
		return ext;
	    }
	  count++;
	  item = item->next;
      }
    return NULL;
}

RL2_DECLARE int
rl2_point_symbolizer_get_graphic_recode_count (rl2PointSymbolizerPtr
					       symbolizer, int index,
					       int *num_items)
{
/* return the Point Symbolizer Color Replacement Items Count */
    int count = 0;
    rl2PrivExternalGraphicPtr ext;
    rl2PrivGraphicItemPtr item;
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->graphic == NULL)
	return RL2_ERROR;
    item = sym->graphic->first;
    while (item != NULL)
      {
	  if (count == index)
	    {
		int cnt = 0;
		rl2PrivColorReplacementPtr repl;
		if (item->type == RL2_EXTERNAL_GRAPHIC && item->item != NULL)
		    ext = (rl2PrivExternalGraphicPtr) (item->item);
		else
		    return RL2_ERROR;
		repl = ext->first;
		while (repl != NULL)
		  {
		      cnt++;
		      repl = repl->next;
		  }
		*num_items = cnt;
		return RL2_OK;
	    }
	  count++;
	  item = item->next;
      }
    return RL2_ERROR;
}

RL2_DECLARE const char *
rl2_point_symbolizer_get_col_graphic_recode_color (rl2PointSymbolizerPtr
						   symbolizer, int index,
						   int repl_index,
						   int *color_index)
{
/* return the Point Symbolizer Table Column Name: Color Replacement */
    int count = 0;
    rl2PrivExternalGraphicPtr ext;
    rl2PrivGraphicItemPtr item;
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->graphic == NULL)
	return NULL;
    item = sym->graphic->first;
    while (item != NULL)
      {
	  if (count == index)
	    {
		int cnt = 0;
		rl2PrivColorReplacementPtr repl;
		if (item->type == RL2_EXTERNAL_GRAPHIC && item->item != NULL)
		    ext = (rl2PrivExternalGraphicPtr) (item->item);
		else
		    return NULL;
		repl = ext->first;
		while (repl != NULL)
		  {
		      if (cnt == repl_index)
			{
			    *color_index = repl->index;
			    return repl->col_color;
			}
		      cnt++;
		      repl = repl->next;
		  }
		return NULL;
	    }
	  count++;
	  item = item->next;
      }
    return NULL;
}

RL2_PRIVATE rl2PrivColorReplacementPtr
rl2_point_symbolizer_get_color_replacement_ref (rl2PointSymbolizerPtr
						symbolizer, int index,
						int repl_index,
						int *color_index)
{
/* return a pointer to a Color Replacement object */
    int count = 0;
    rl2PrivExternalGraphicPtr ext;
    rl2PrivGraphicItemPtr item;
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->graphic == NULL)
	return NULL;
    item = sym->graphic->first;
    while (item != NULL)
      {
	  if (count == index)
	    {
		int cnt = 0;
		rl2PrivColorReplacementPtr repl;
		if (item->type == RL2_EXTERNAL_GRAPHIC && item->item != NULL)
		    ext = (rl2PrivExternalGraphicPtr) (item->item);
		else
		    return NULL;
		repl = ext->first;
		while (repl != NULL)
		  {
		      if (cnt == repl_index)
			{
			    *color_index = repl->index;
			    return repl;
			}
		      cnt++;
		      repl = repl->next;
		  }
		return NULL;
	    }
	  count++;
	  item = item->next;
      }
    return NULL;
}

RL2_DECLARE int
rl2_point_symbolizer_is_mark (rl2PointSymbolizerPtr symbolizer, int index,
			      int *mark)
{
/* checks if a Point Symbolizer item is a Mark */
    int count = 0;
    rl2PrivGraphicItemPtr item;
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->graphic == NULL)
	return RL2_ERROR;
    item = sym->graphic->first;
    while (item != NULL)
      {
	  if (count == index)
	    {
		if (item->type == RL2_MARK_GRAPHIC && item->item != NULL)
		    *mark = 1;
		else
		    *mark = 0;
		return RL2_OK;
	    }
	  count++;
	  item = item->next;
      }
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_point_symbolizer_mark_get_well_known_type (rl2PointSymbolizerPtr
					       symbolizer, int index,
					       unsigned char *type)
{
/* return the Point Symbolizer Mark WellKnownType */
    int count = 0;
    rl2PrivMarkPtr mark;
    rl2PrivGraphicItemPtr item;
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->graphic == NULL)
	return RL2_ERROR;
    item = sym->graphic->first;
    while (item != NULL)
      {
	  if (count == index)
	    {
		if (item->type == RL2_MARK_GRAPHIC && item->item != NULL)
		    mark = (rl2PrivMarkPtr) (item->item);
		else
		    return RL2_ERROR;
		switch (mark->well_known_type)
		  {
		  case RL2_GRAPHIC_MARK_SQUARE:
		  case RL2_GRAPHIC_MARK_CIRCLE:
		  case RL2_GRAPHIC_MARK_TRIANGLE:
		  case RL2_GRAPHIC_MARK_STAR:
		  case RL2_GRAPHIC_MARK_CROSS:
		  case RL2_GRAPHIC_MARK_X:
		      *type = mark->well_known_type;
		      break;
		  default:
		      *type = RL2_GRAPHIC_MARK_UNKNOWN;
		      break;
		  };
		return RL2_OK;
	    }
	  count++;
	  item = item->next;
      }
    return RL2_ERROR;
}

RL2_PRIVATE rl2PrivMarkPtr
rl2_point_symbolizer_get_mark_ref (rl2PointSymbolizerPtr symbolizer, int index)
{
/* return a pointer to a Mark object */
    int count = 0;
    rl2PrivMarkPtr mark;
    rl2PrivGraphicItemPtr item;
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->graphic == NULL)
	return NULL;
    item = sym->graphic->first;
    while (item != NULL)
      {
	  if (count == index)
	    {
		if (item->type == RL2_MARK_GRAPHIC && item->item != NULL)
		    mark = (rl2PrivMarkPtr) (item->item);
		else
		    return NULL;
		return mark;
	    }
	  count++;
	  item = item->next;
      }
    return NULL;
}

RL2_DECLARE const char *
rl2_point_symbolizer_mark_get_col_well_known_type (rl2PointSymbolizerPtr
						   symbolizer, int index)
{
/* return the Point Symbolizer Mark Table Column Name: WellKnownType */
    int count = 0;
    rl2PrivMarkPtr mark;
    rl2PrivGraphicItemPtr item;
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->graphic == NULL)
	return NULL;
    item = sym->graphic->first;
    while (item != NULL)
      {
	  if (count == index)
	    {
		if (item->type == RL2_MARK_GRAPHIC && item->item != NULL)
		    mark = (rl2PrivMarkPtr) (item->item);
		else
		    return NULL;
		return mark->col_mark_type;
	    }
	  count++;
	  item = item->next;
      }
    return NULL;
}

RL2_DECLARE int
rl2_point_symbolizer_mark_has_stroke (rl2PointSymbolizerPtr symbolizer,
				      int index, int *stroke)
{
/* checks if a Point Symbolizer Mark has a Stroke */
    int count = 0;
    rl2PrivMarkPtr mark;
    rl2PrivGraphicItemPtr item;
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->graphic == NULL)
	return RL2_ERROR;
    item = sym->graphic->first;
    while (item != NULL)
      {
	  if (count == index)
	    {
		if (item->type == RL2_MARK_GRAPHIC && item->item != NULL)
		    mark = (rl2PrivMarkPtr) (item->item);
		else
		    return RL2_ERROR;
		if (mark->stroke == NULL)
		    *stroke = 0;
		else
		    *stroke = 1;
		return RL2_OK;
	    }
	  count++;
	  item = item->next;
      }
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_point_symbolizer_mark_get_stroke_color (rl2PointSymbolizerPtr symbolizer,
					    int index, unsigned char *red,
					    unsigned char *green,
					    unsigned char *blue)
{
/* return the Point Symbolizer Mark Stroke RGB color */
    int count = 0;
    rl2PrivMarkPtr mark;
    rl2PrivGraphicItemPtr item;
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->graphic == NULL)
	return RL2_ERROR;
    item = sym->graphic->first;
    while (item != NULL)
      {
	  if (count == index)
	    {
		if (item->type == RL2_MARK_GRAPHIC && item->item != NULL)
		    mark = (rl2PrivMarkPtr) (item->item);
		else
		    return RL2_ERROR;
		if (mark->stroke == NULL)
		    return RL2_ERROR;
		*red = mark->stroke->red;
		*green = mark->stroke->green;
		*blue = mark->stroke->blue;
		return RL2_OK;
	    }
	  count++;
	  item = item->next;
      }
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_point_symbolizer_mark_get_stroke_width (rl2PointSymbolizerPtr symbolizer,
					    int index, double *width)
{
/* return the Point Symbolizer Mark Stroke width */
    int count = 0;
    rl2PrivMarkPtr mark;
    rl2PrivGraphicItemPtr item;
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->graphic == NULL)
	return RL2_ERROR;
    item = sym->graphic->first;
    while (item != NULL)
      {
	  if (count == index)
	    {
		if (item->type == RL2_MARK_GRAPHIC && item->item != NULL)
		    mark = (rl2PrivMarkPtr) (item->item);
		else
		    return RL2_ERROR;
		if (mark->stroke == NULL)
		    return RL2_ERROR;
		*width = mark->stroke->width;
		return RL2_OK;
	    }
	  count++;
	  item = item->next;
      }
    return RL2_ERROR;
}

RL2_DECLARE const char *
rl2_point_symbolizer_mark_get_col_stroke_color (rl2PointSymbolizerPtr
						symbolizer, int index)
{
/* return the Point Symbolizer Mark Table Column Name: Stroke Color */
    int count = 0;
    rl2PrivMarkPtr mark;
    rl2PrivGraphicItemPtr item;
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->graphic == NULL)
	return NULL;
    item = sym->graphic->first;
    while (item != NULL)
      {
	  if (count == index)
	    {
		if (item->type == RL2_MARK_GRAPHIC && item->item != NULL)
		    mark = (rl2PrivMarkPtr) (item->item);
		else
		    return NULL;
		if (mark->stroke == NULL)
		    return NULL;
		return mark->stroke->col_color;
	    }
	  count++;
	  item = item->next;
      }
    return NULL;
}

RL2_DECLARE const char *
rl2_point_symbolizer_mark_get_col_stroke_width (rl2PointSymbolizerPtr
						symbolizer, int index)
{
/* return the Point Symbolizer Mark Table Column Name: Stroke Width */
    int count = 0;
    rl2PrivMarkPtr mark;
    rl2PrivGraphicItemPtr item;
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->graphic == NULL)
	return NULL;
    item = sym->graphic->first;
    while (item != NULL)
      {
	  if (count == index)
	    {
		if (item->type == RL2_MARK_GRAPHIC && item->item != NULL)
		    mark = (rl2PrivMarkPtr) (item->item);
		else
		    return NULL;
		if (mark->stroke == NULL)
		    return NULL;
		return mark->stroke->col_width;
	    }
	  count++;
	  item = item->next;
      }
    return NULL;
}

RL2_DECLARE int
rl2_point_symbolizer_mark_get_stroke_linejoin (rl2PointSymbolizerPtr
					       symbolizer, int index,
					       unsigned char *linejoin)
{
/* return the Point Symbolizer Mark Stroke Linejoin mode */
    int count = 0;
    rl2PrivMarkPtr mark;
    rl2PrivGraphicItemPtr item;
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->graphic == NULL)
	return RL2_ERROR;
    item = sym->graphic->first;
    while (item != NULL)
      {
	  if (count == index)
	    {
		if (item->type == RL2_MARK_GRAPHIC && item->item != NULL)
		    mark = (rl2PrivMarkPtr) (item->item);
		else
		    return RL2_ERROR;
		if (mark->stroke == NULL)
		    return RL2_ERROR;
		*linejoin = mark->stroke->linejoin;
		return RL2_OK;
	    }
	  count++;
	  item = item->next;
      }
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_point_symbolizer_mark_get_stroke_linecap (rl2PointSymbolizerPtr
					      symbolizer, int index,
					      unsigned char *linecap)
{
/* return the Point Symbolizer Stroke Mark Linecap mode */
    int count = 0;
    rl2PrivMarkPtr mark;
    rl2PrivGraphicItemPtr item;
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->graphic == NULL)
	return RL2_ERROR;
    item = sym->graphic->first;
    while (item != NULL)
      {
	  if (count == index)
	    {
		if (item->type == RL2_MARK_GRAPHIC && item->item != NULL)
		    mark = (rl2PrivMarkPtr) (item->item);
		else
		    return RL2_ERROR;
		if (mark->stroke == NULL)
		    return RL2_ERROR;
		*linecap = mark->stroke->linecap;
		return RL2_OK;
	    }
	  count++;
	  item = item->next;
      }
    return RL2_ERROR;
}

RL2_DECLARE const char *
rl2_point_symbolizer_mark_get_col_stroke_linejoin (rl2PointSymbolizerPtr
						   symbolizer, int index)
{
/* return the Point Symbolizer Mark Table Column Name: Stroke LineJoin */
    int count = 0;
    rl2PrivMarkPtr mark;
    rl2PrivGraphicItemPtr item;
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->graphic == NULL)
	return NULL;
    item = sym->graphic->first;
    while (item != NULL)
      {
	  if (count == index)
	    {
		if (item->type == RL2_MARK_GRAPHIC && item->item != NULL)
		    mark = (rl2PrivMarkPtr) (item->item);
		else
		    return NULL;
		if (mark->stroke == NULL)
		    return NULL;
		return mark->stroke->col_join;
	    }
	  count++;
	  item = item->next;
      }
    return NULL;
}

RL2_DECLARE const char *
rl2_point_symbolizer_mark_get_col_stroke_linecap (rl2PointSymbolizerPtr
						  symbolizer, int index)
{
/* return the Point Symbolizer Mark Table Column Name: Stroke LineCap */
    int count = 0;
    rl2PrivMarkPtr mark;
    rl2PrivGraphicItemPtr item;
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->graphic == NULL)
	return NULL;
    item = sym->graphic->first;
    while (item != NULL)
      {
	  if (count == index)
	    {
		if (item->type == RL2_MARK_GRAPHIC && item->item != NULL)
		    mark = (rl2PrivMarkPtr) (item->item);
		else
		    return NULL;
		if (mark->stroke == NULL)
		    return NULL;
		return mark->stroke->col_cap;
	    }
	  count++;
	  item = item->next;
      }
    return NULL;
}

RL2_DECLARE int
rl2_point_symbolizer_mark_get_stroke_dash_count (rl2PointSymbolizerPtr
						 symbolizer, int index,
						 int *cnt)
{
/* return the Point Symbolizer Mark Stroke Dash count */
    int count = 0;
    rl2PrivMarkPtr mark;
    rl2PrivGraphicItemPtr item;
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->graphic == NULL)
	return RL2_ERROR;
    item = sym->graphic->first;
    while (item != NULL)
      {
	  if (count == index)
	    {
		if (item->type == RL2_MARK_GRAPHIC && item->item != NULL)
		    mark = (rl2PrivMarkPtr) (item->item);
		else
		    return RL2_ERROR;
		if (mark->stroke == NULL)
		    return RL2_ERROR;
		*cnt = mark->stroke->dash_count;
		return RL2_OK;
	    }
	  count++;
	  item = item->next;
      }
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_point_symbolizer_mark_get_stroke_dash_item (rl2PointSymbolizerPtr
						symbolizer, int index,
						int item_index, double *item)
{
/* return a Point Symbolizer Mark Stroke Dash item */
    int count = 0;
    rl2PrivMarkPtr mark;
    rl2PrivGraphicItemPtr itm;
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->graphic == NULL)
	return RL2_ERROR;
    itm = sym->graphic->first;
    while (itm != NULL)
      {
	  if (count == index)
	    {
		if (itm->type == RL2_MARK_GRAPHIC && itm->item != NULL)
		    mark = (rl2PrivMarkPtr) (itm->item);
		else
		    return RL2_ERROR;
		if (mark->stroke == NULL)
		    return RL2_ERROR;
		if (mark->stroke->dash_list == NULL)
		    return RL2_ERROR;
		if (item_index >= 0 && item_index < mark->stroke->dash_count)
		    ;
		else
		    return RL2_ERROR;
		*item = *(mark->stroke->dash_list + item_index);
		return RL2_OK;
	    }
	  count++;
	  itm = itm->next;
      }
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_point_symbolizer_mark_get_stroke_dash_offset (rl2PointSymbolizerPtr
						  symbolizer, int index,
						  double *offset)
{
/* return the Point Symbolizer Mark Stroke Dash initial offset */
    int count = 0;
    rl2PrivMarkPtr mark;
    rl2PrivGraphicItemPtr itm;
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->graphic == NULL)
	return RL2_ERROR;
    itm = sym->graphic->first;
    while (itm != NULL)
      {
	  if (count == index)
	    {
		if (itm->type == RL2_MARK_GRAPHIC && itm->item != NULL)
		    mark = (rl2PrivMarkPtr) (itm->item);
		else
		    return RL2_ERROR;
		if (mark->stroke == NULL)
		    return RL2_ERROR;
		*offset = mark->stroke->dash_offset;
		return RL2_OK;
	    }
	  count++;
	  itm = itm->next;
      }
    return RL2_ERROR;
}

RL2_DECLARE const char *
rl2_point_symbolizer_mark_get_col_stroke_dash_array (rl2PointSymbolizerPtr
						     symbolizer, int index)
{
/* return the Point Symbolizer Mark Table Column Name: Stroke Dash Array */
    int count = 0;
    rl2PrivMarkPtr mark;
    rl2PrivGraphicItemPtr item;
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->graphic == NULL)
	return NULL;
    item = sym->graphic->first;
    while (item != NULL)
      {
	  if (count == index)
	    {
		if (item->type == RL2_MARK_GRAPHIC && item->item != NULL)
		    mark = (rl2PrivMarkPtr) (item->item);
		else
		    return NULL;
		if (mark->stroke == NULL)
		    return NULL;
		return mark->stroke->col_dash;
	    }
	  count++;
	  item = item->next;
      }
    return NULL;
}

RL2_DECLARE const char *
rl2_point_symbolizer_mark_get_col_stroke_dash_offset (rl2PointSymbolizerPtr
						      symbolizer, int index)
{
/* return the Point Symbolizer Mark Table Column Name: Stroke Dash Offset */
    int count = 0;
    rl2PrivMarkPtr mark;
    rl2PrivGraphicItemPtr item;
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->graphic == NULL)
	return NULL;
    item = sym->graphic->first;
    while (item != NULL)
      {
	  if (count == index)
	    {
		if (item->type == RL2_MARK_GRAPHIC && item->item != NULL)
		    mark = (rl2PrivMarkPtr) (item->item);
		else
		    return NULL;
		if (mark->stroke == NULL)
		    return NULL;
		return mark->stroke->col_dashoff;
	    }
	  count++;
	  item = item->next;
      }
    return NULL;
}

RL2_DECLARE int
rl2_point_symbolizer_mark_has_fill (rl2PointSymbolizerPtr symbolizer,
				    int index, int *fill)
{
/* checks if a Point Symbolizer Mark has a Fill */
    int count = 0;
    rl2PrivMarkPtr mark;
    rl2PrivGraphicItemPtr item;
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->graphic == NULL)
	return RL2_ERROR;
    item = sym->graphic->first;
    while (item != NULL)
      {
	  if (count == index)
	    {
		if (item->type == RL2_MARK_GRAPHIC && item->item != NULL)
		    mark = (rl2PrivMarkPtr) (item->item);
		else
		    return RL2_ERROR;
		if (mark->fill == NULL)
		    *fill = 0;
		else
		    *fill = 1;
		return RL2_OK;
	    }
	  count++;
	  item = item->next;
      }
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_point_symbolizer_mark_get_fill_color (rl2PointSymbolizerPtr symbolizer,
					  int index, unsigned char *red,
					  unsigned char *green,
					  unsigned char *blue)
{
/* return the Point Symbolizer Mark Fill RGB color */
    int count = 0;
    rl2PrivMarkPtr mark;
    rl2PrivGraphicItemPtr item;
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->graphic == NULL)
	return RL2_ERROR;
    item = sym->graphic->first;
    while (item != NULL)
      {
	  if (count == index)
	    {
		if (item->type == RL2_MARK_GRAPHIC && item->item != NULL)
		    mark = (rl2PrivMarkPtr) (item->item);
		else
		    return RL2_ERROR;
		if (mark->fill == NULL)
		    return RL2_ERROR;
		*red = mark->fill->red;
		*green = mark->fill->green;
		*blue = mark->fill->blue;
		return RL2_OK;
	    }
	  count++;
	  item = item->next;
      }
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_point_symbolizer_get_opacity (rl2PointSymbolizerPtr symbolizer,
				  double *opacity)
{
/* return a Point Symbolizer Opacity */
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->graphic == NULL)
	return RL2_ERROR;
    *opacity = sym->graphic->opacity;
    return RL2_OK;
}

RL2_DECLARE int
rl2_point_symbolizer_get_size (rl2PointSymbolizerPtr symbolizer, double *size)
{
/* return a Point Symbolizer Size */
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->graphic == NULL)
	return RL2_ERROR;
    *size = sym->graphic->size;
    return RL2_OK;
}

RL2_DECLARE int
rl2_point_symbolizer_get_rotation (rl2PointSymbolizerPtr symbolizer,
				   double *rotation)
{
/* return a Point Symbolizer Rotation */
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->graphic == NULL)
	return RL2_ERROR;
    *rotation = sym->graphic->rotation;
    return RL2_OK;
}

RL2_DECLARE int
rl2_point_symbolizer_get_anchor_point (rl2PointSymbolizerPtr symbolizer,
				       double *x, double *y)
{
/* return a Point Symbolizer Anchor Point */
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->graphic == NULL)
	return RL2_ERROR;
    *x = sym->graphic->anchor_point_x;
    *y = sym->graphic->anchor_point_y;
    return RL2_OK;
}

RL2_DECLARE int
rl2_point_symbolizer_get_displacement (rl2PointSymbolizerPtr symbolizer,
				       double *x, double *y)
{
/* return a Point Symbolizer Displacement */
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return RL2_ERROR;
    if (sym->graphic == NULL)
	return RL2_ERROR;
    *x = sym->graphic->displacement_x;
    *y = sym->graphic->displacement_y;
    return RL2_OK;
}

RL2_DECLARE const char *
rl2_point_symbolizer_mark_get_col_fill_color (rl2PointSymbolizerPtr symbolizer,
					      int index)
{
/* return the Point Symbolizer Mark Table Column Name: Fill Color */
    int count = 0;
    rl2PrivMarkPtr mark;
    rl2PrivGraphicItemPtr item;
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->graphic == NULL)
	return NULL;
    item = sym->graphic->first;
    while (item != NULL)
      {
	  if (count == index)
	    {
		if (item->type == RL2_MARK_GRAPHIC && item->item != NULL)
		    mark = (rl2PrivMarkPtr) (item->item);
		else
		    return NULL;
		if (mark->fill == NULL)
		    return NULL;
		return mark->fill->col_color;
	    }
	  count++;
	  item = item->next;
      }
    return NULL;
}

RL2_DECLARE const char *
rl2_point_symbolizer_get_col_opacity (rl2PointSymbolizerPtr symbolizer)
{
/* return a Point Symbolizer Table Column Name: Opacity */
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->graphic == NULL)
	return NULL;
    return sym->graphic->col_opacity;
    return RL2_OK;
}

RL2_DECLARE const char *
rl2_point_symbolizer_get_col_size (rl2PointSymbolizerPtr symbolizer)
{
/* return a Point Symbolizer Table Column Name: Size */
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->graphic == NULL)
	return NULL;
    return sym->graphic->col_size;
}

RL2_DECLARE const char *
rl2_point_symbolizer_get_col_rotation (rl2PointSymbolizerPtr symbolizer)
{
/* return a Point Symbolizer Table Column Name: Rotation */
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->graphic == NULL)
	return NULL;
    return sym->graphic->col_rotation;
}

RL2_DECLARE const char *
rl2_point_symbolizer_get_col_anchor_point_x (rl2PointSymbolizerPtr symbolizer)
{
/* return a Point Symbolizer Table Column Name: Anchor Point X */
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->graphic == NULL)
	return NULL;
    return sym->graphic->col_point_x;
}

RL2_DECLARE const char *
rl2_point_symbolizer_get_col_anchor_point_y (rl2PointSymbolizerPtr symbolizer)
{
/* return a Point Symbolizer Table Column Name: Anchor Point Y */
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->graphic == NULL)
	return NULL;
    return sym->graphic->col_point_y;
}

RL2_DECLARE const char *
rl2_point_symbolizer_get_col_displacement_x (rl2PointSymbolizerPtr symbolizer)
{
/* return a Point Symbolizer Table Column Name: Displacement X */
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->graphic == NULL)
	return NULL;
    return sym->graphic->col_displ_x;
}

RL2_DECLARE const char *
rl2_point_symbolizer_get_col_displacement_y (rl2PointSymbolizerPtr symbolizer)
{
/* return a Point Symbolizer Table Column Name: Displacement Y */
    rl2PrivPointSymbolizerPtr sym = (rl2PrivPointSymbolizerPtr) symbolizer;
    if (sym == NULL)
	return NULL;
    if (sym->graphic == NULL)
	return NULL;
    return sym->graphic->col_displ_y;
}

RL2_PRIVATE void
rl2_destroy_color_replacement (rl2PrivColorReplacementPtr repl)
{
/* destroying a Color Replacement object */
    if (repl == NULL)
	return;
    if (repl->col_color != NULL)
	free (repl->col_color);
    free (repl);
}

RL2_PRIVATE void
rl2_destroy_external_graphic (rl2PrivExternalGraphicPtr ext)
{
/* destroying an External Graphic object */
    rl2PrivColorReplacementPtr repl;
    rl2PrivColorReplacementPtr replN;
    if (ext == NULL)
	return;
    if (ext->xlink_href != NULL)
	free (ext->xlink_href);
    if (ext->col_href != NULL)
	free (ext->col_href);
    repl = ext->first;
    while (repl != NULL)
      {
	  replN = repl->next;
	  rl2_destroy_color_replacement (repl);
	  repl = replN;
      }
    free (ext);
}

RL2_PRIVATE void
rl2_destroy_mark (rl2PrivMarkPtr mark)
{
/* destroying a Mark object */
    if (mark == NULL)
	return;
    if (mark->stroke != NULL)
	rl2_destroy_stroke (mark->stroke);
    if (mark->fill != NULL)
	rl2_destroy_fill (mark->fill);
    if (mark->col_mark_type != NULL)
	free (mark->col_mark_type);
    free (mark);
}

RL2_PRIVATE void
rl2_destroy_graphic_item (rl2PrivGraphicItemPtr item)
{
/* destroying a Graphic Item object */
    if (item == NULL)
	return;
    if (item->type == RL2_EXTERNAL_GRAPHIC)
	rl2_destroy_external_graphic ((rl2PrivExternalGraphicPtr) (item->item));
    if (item->type == RL2_MARK_GRAPHIC)
	rl2_destroy_mark ((rl2PrivMarkPtr) (item->item));
    free (item);
}

RL2_PRIVATE void
rl2_destroy_graphic (rl2PrivGraphicPtr graphic)
{
/* destroying a Graphic object */
    rl2PrivGraphicItemPtr item;
    rl2PrivGraphicItemPtr itemN;
    if (graphic == NULL)
	return;
    item = graphic->first;
    while (item != NULL)
      {
	  itemN = item->next;
	  rl2_destroy_graphic_item (item);
	  item = itemN;
      }
    if (graphic->col_opacity != NULL)
	free (graphic->col_opacity);
    if (graphic->col_rotation != NULL)
	free (graphic->col_rotation);
    if (graphic->col_size != NULL)
	free (graphic->col_size);
    if (graphic->col_point_x != NULL)
	free (graphic->col_point_x);
    if (graphic->col_point_y != NULL)
	free (graphic->col_point_y);
    if (graphic->col_displ_x != NULL)
	free (graphic->col_displ_x);
    if (graphic->col_displ_y != NULL)
	free (graphic->col_displ_y);
    free (graphic);
}

RL2_PRIVATE void
rl2_destroy_stroke (rl2PrivStrokePtr stroke)
{
/* destroying a Stroke object */
    if (stroke == NULL)
	return;
    if (stroke->graphic != NULL)
	rl2_destroy_graphic (stroke->graphic);
    if (stroke->dash_list != NULL)
	free (stroke->dash_list);
    if (stroke->col_color != NULL)
	free (stroke->col_color);
    if (stroke->col_opacity != NULL)
	free (stroke->col_opacity);
    if (stroke->col_width != NULL)
	free (stroke->col_width);
    if (stroke->col_join != NULL)
	free (stroke->col_join);
    if (stroke->col_cap != NULL)
	free (stroke->col_cap);
    if (stroke->col_dash != NULL)
	free (stroke->col_dash);
    if (stroke->col_dashoff != NULL)
	free (stroke->col_dashoff);
    free (stroke);
}

RL2_PRIVATE void
rl2_destroy_fill (rl2PrivFillPtr fill)
{
/* destroying a Fill object */
    if (fill == NULL)
	return;
    if (fill->graphic != NULL)
	rl2_destroy_graphic (fill->graphic);
    if (fill->col_color != NULL)
	free (fill->col_color);
    if (fill->col_opacity != NULL)
	free (fill->col_opacity);
    free (fill);
}

RL2_PRIVATE void
rl2_destroy_point_symbolizer (rl2PrivPointSymbolizerPtr ptr)
{
/* destroying a Point Symbolizer */
    if (ptr == NULL)
	return;
    if (ptr->graphic != NULL)
	rl2_destroy_graphic (ptr->graphic);
    free (ptr);
}

RL2_PRIVATE void
rl2_destroy_line_symbolizer (rl2PrivLineSymbolizerPtr ptr)
{
/* destroying a Line Symbolizer */
    if (ptr == NULL)
	return;
    if (ptr->stroke != NULL)
	rl2_destroy_stroke (ptr->stroke);
    if (ptr->col_perpoff != NULL)
	free (ptr->col_perpoff);
    free (ptr);
}

RL2_PRIVATE void
rl2_destroy_polygon_symbolizer (rl2PrivPolygonSymbolizerPtr ptr)
{
/* destroying a Polygon Symbolizer */
    if (ptr == NULL)
	return;
    if (ptr->stroke != NULL)
	rl2_destroy_stroke (ptr->stroke);
    if (ptr->fill != NULL)
	rl2_destroy_fill (ptr->fill);
    if (ptr->col_displ_x != NULL)
	free (ptr->col_displ_x);
    if (ptr->col_displ_y != NULL)
	free (ptr->col_displ_y);
    if (ptr->col_perpoff != NULL)
	free (ptr->col_perpoff);
    free (ptr);
}

RL2_PRIVATE void
rl2_destroy_point_placement (rl2PrivPointPlacementPtr place)
{
/* destroying a PointPlacement object */
    if (place == NULL)
	return;
    if (place->col_point_x != NULL)
	free (place->col_point_x);
    if (place->col_point_y != NULL)
	free (place->col_point_y);
    if (place->col_displ_x != NULL)
	free (place->col_displ_x);
    if (place->col_displ_y != NULL)
	free (place->col_displ_y);
    if (place->col_rotation != NULL)
	free (place->col_rotation);
    free (place);
}

RL2_PRIVATE void
rl2_destroy_line_placement (rl2PrivLinePlacementPtr place)
{
/* destroying a LinePlacement object */
    if (place == NULL)
	return;
    if (place->col_perpoff != NULL)
	free (place->col_perpoff);
    if (place->col_inigap != NULL)
	free (place->col_inigap);
    if (place->col_gap != NULL)
	free (place->col_gap);
    free (place);
}

RL2_PRIVATE void
rl2_destroy_halo (rl2PrivHaloPtr halo)
{
/* destroying an Halo object */
    if (halo == NULL)
	return;
    if (halo->fill != NULL)
	rl2_destroy_fill (halo->fill);
    if (halo->col_radius != NULL)
	free (halo->col_radius);
    free (halo);
}

RL2_PRIVATE void
rl2_destroy_text_symbolizer (rl2PrivTextSymbolizerPtr ptr)
{
/* destroying a Text Symbolizer */
    int i;
    if (ptr == NULL)
	return;
    if (ptr->label != NULL)
	free (ptr->label);
    if (ptr->col_label != NULL)
	free (ptr->col_label);
    if (ptr->col_font != NULL)
	free (ptr->col_font);
    if (ptr->col_style != NULL)
	free (ptr->col_style);
    if (ptr->col_weight != NULL)
	free (ptr->col_weight);
    if (ptr->col_size != NULL)
	free (ptr->col_size);
    for (i = 0; i < RL2_MAX_FONT_FAMILIES; i++)
      {
	  if (*(ptr->font_families + i) != NULL)
	      free (*(ptr->font_families + i));
      }
    if (ptr->label_placement_type == RL2_LABEL_PLACEMENT_POINT
	&& ptr->label_placement != NULL)
	rl2_destroy_point_placement ((rl2PrivPointPlacementPtr)
				     (ptr->label_placement));
    if (ptr->label_placement_type == RL2_LABEL_PLACEMENT_LINE
	&& ptr->label_placement != NULL)
	rl2_destroy_line_placement ((rl2PrivLinePlacementPtr)
				    (ptr->label_placement));
    if (ptr->halo != NULL)
	rl2_destroy_halo (ptr->halo);
    if (ptr->fill != NULL)
	rl2_destroy_fill (ptr->fill);
    free (ptr);
}

RL2_PRIVATE void
rl2_destroy_vector_symbolizer_item (rl2PrivVectorSymbolizerItemPtr item)
{
/* destroying a VectorSymbolizerItem object */
    if (item == NULL)
	return;

    if (item->symbolizer_type == RL2_POINT_SYMBOLIZER)
	rl2_destroy_point_symbolizer ((rl2PrivPointSymbolizerPtr)
				      (item->symbolizer));
    if (item->symbolizer_type == RL2_LINE_SYMBOLIZER)
	rl2_destroy_line_symbolizer ((rl2PrivLineSymbolizerPtr)
				     (item->symbolizer));
    if (item->symbolizer_type == RL2_POLYGON_SYMBOLIZER)
	rl2_destroy_polygon_symbolizer ((rl2PrivPolygonSymbolizerPtr)
					(item->symbolizer));
    if (item->symbolizer_type == RL2_TEXT_SYMBOLIZER)
	rl2_destroy_text_symbolizer ((rl2PrivTextSymbolizerPtr)
				     (item->symbolizer));
    free (item);
}

RL2_PRIVATE void
rl2_destroy_vector_symbolizer (rl2PrivVectorSymbolizerPtr symbolizer)
{
/* destroying a VectorSymbolizer object */
    rl2PrivVectorSymbolizerItemPtr item;
    rl2PrivVectorSymbolizerItemPtr itemN;
    if (symbolizer == NULL)
	return;

    item = symbolizer->first;
    while (item != NULL)
      {
	  itemN = item->next;
	  rl2_destroy_vector_symbolizer_item (item);
	  item = itemN;
      }
    free (symbolizer);
}

RL2_DECLARE void
rl2_destroy_group_style (rl2GroupStylePtr style)
{
/* destroying a Group Style object */
    rl2PrivChildStylePtr child;
    rl2PrivChildStylePtr child_n;
    rl2PrivGroupStylePtr stl = (rl2PrivGroupStylePtr) style;
    if (stl == NULL)
	return;

    if (stl->name != NULL)
	free (stl->name);
    child = stl->first;
    while (child != NULL)
      {
	  child_n = child->next;
	  if (child->namedLayer != NULL)
	      free (child->namedLayer);
	  if (child->namedStyle != NULL)
	      free (child->namedStyle);
	  free (child);
	  child = child_n;
      }
    free (stl);
}

RL2_DECLARE const char *
rl2_get_group_style_name (rl2GroupStylePtr style)
{
/* return the Group Style Name */
    rl2PrivGroupStylePtr stl = (rl2PrivGroupStylePtr) style;
    if (stl == NULL)
	return NULL;
    return stl->name;
}

RL2_DECLARE int
rl2_is_valid_group_style (rl2GroupStylePtr style, int *valid)
{
/* testing a Group Style for validity */
    rl2PrivGroupStylePtr stl = (rl2PrivGroupStylePtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    *valid = stl->valid;
    return RL2_OK;
}

RL2_DECLARE int
rl2_get_group_style_count (rl2GroupStylePtr style, int *count)
{
/* return the total count of Group Style Items */
    int cnt = 0;
    rl2PrivChildStylePtr child;
    rl2PrivGroupStylePtr stl = (rl2PrivGroupStylePtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    child = stl->first;
    while (child != NULL)
      {
	  /* counting how many Children */
	  cnt++;
	  child = child->next;
      }
    *count = cnt;
    return RL2_OK;
}

RL2_DECLARE const char *
rl2_get_group_prefix (rl2GroupStylePtr style, int index)
{
/* return the Nth DbPrefix from a Group Style */
    int cnt = 0;
    const char *str = NULL;
    rl2PrivChildStylePtr child;
    rl2PrivGroupStylePtr stl = (rl2PrivGroupStylePtr) style;
    if (stl == NULL)
	return NULL;
    if (index < 0)
	return NULL;
    child = stl->first;
    while (child != NULL)
      {
	  /* counting how many Children */
	  cnt++;
	  child = child->next;
      }
    if (index >= cnt)
	return NULL;
    cnt = 0;
    child = stl->first;
    while (child != NULL)
      {
	  if (cnt == index)
	    {
		str = child->dbPrefix;
		break;
	    }
	  cnt++;
	  child = child->next;
      }
    return str;
}

RL2_DECLARE const char *
rl2_get_group_named_layer (rl2GroupStylePtr style, int index)
{
/* return the Nth NamedLayer from a Group Style */
    int cnt = 0;
    const char *str = NULL;
    rl2PrivChildStylePtr child;
    rl2PrivGroupStylePtr stl = (rl2PrivGroupStylePtr) style;
    if (stl == NULL)
	return NULL;
    if (index < 0)
	return NULL;
    child = stl->first;
    while (child != NULL)
      {
	  /* counting how many Children */
	  cnt++;
	  child = child->next;
      }
    if (index >= cnt)
	return NULL;
    cnt = 0;
    child = stl->first;
    while (child != NULL)
      {
	  if (cnt == index)
	    {
		str = child->namedLayer;
		break;
	    }
	  cnt++;
	  child = child->next;
      }
    return str;
}

RL2_DECLARE const char *
rl2_get_group_named_style (rl2GroupStylePtr style, int index)
{
/* return the Nth NamedStyle from a Group Style */
    int cnt = 0;
    const char *str = NULL;
    rl2PrivChildStylePtr child;
    rl2PrivGroupStylePtr stl = (rl2PrivGroupStylePtr) style;
    if (stl == NULL)
	return NULL;
    if (index < 0)
	return NULL;
    child = stl->first;
    while (child != NULL)
      {
	  /* counting how many Children */
	  cnt++;
	  child = child->next;
      }
    if (index >= cnt)
	return NULL;
    cnt = 0;
    child = stl->first;
    while (child != NULL)
      {
	  if (cnt == index)
	    {
		str = child->namedStyle;
		break;
	    }
	  cnt++;
	  child = child->next;
      }
    return str;
}

RL2_DECLARE int
rl2_is_valid_group_named_layer (rl2GroupStylePtr style, int index, int *valid)
{
/* testing for validity the Nth NamedLayer from a Group Style */
    int cnt = 0;
    rl2PrivChildStylePtr child;
    rl2PrivGroupStylePtr stl = (rl2PrivGroupStylePtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (index < 0)
	return RL2_ERROR;
    child = stl->first;
    while (child != NULL)
      {
	  /* counting how many Children */
	  cnt++;
	  child = child->next;
      }
    if (index >= cnt)
	return RL2_ERROR;
    cnt = 0;
    child = stl->first;
    while (child != NULL)
      {
	  if (cnt == index)
	    {
		*valid = child->validLayer;
		break;
	    }
	  cnt++;
	  child = child->next;
      }
    return RL2_OK;
}

RL2_DECLARE int
rl2_is_valid_group_named_style (rl2GroupStylePtr style, int index, int *valid)
{
/* testing for validity the Nth NamedStyle from a Group Style */
    int cnt = 0;
    rl2PrivChildStylePtr child;
    rl2PrivGroupStylePtr stl = (rl2PrivGroupStylePtr) style;
    if (stl == NULL)
	return RL2_ERROR;
    if (index < 0)
	return RL2_ERROR;
    child = stl->first;
    while (child != NULL)
      {
	  /* counting how many Children */
	  cnt++;
	  child = child->next;
      }
    if (index >= cnt)
	return RL2_ERROR;
    cnt = 0;
    child = stl->first;
    while (child != NULL)
      {
	  if (cnt == index)
	    {
		*valid = child->validStyle;
		break;
	    }
	  cnt++;
	  child = child->next;
      }
    return RL2_OK;
}

RL2_DECLARE void
rl2_destroy_group_renderer (rl2GroupRendererPtr group)
{
/* memory cleanup - destroying a GroupRenderer object */
    int i;
    rl2PrivGroupRendererPtr ptr = (rl2PrivGroupRendererPtr) group;
    if (ptr == NULL)
	return;
    for (i = 0; i < ptr->count; i++)
      {
	  rl2PrivGroupRendererLayerPtr lyr = ptr->layers + i;
	  if (lyr->layer_name != NULL)
	      free (lyr->layer_name);
	  if (lyr->coverage != NULL)
	      rl2_destroy_coverage (lyr->coverage);
	  /*
	     if (lyr->coverage_style != NULL)
	     rl2_destroy_coverage_style (lyr->coverage_style);
	   */
	  if (lyr->raster_stats != NULL)
	      rl2_destroy_raster_statistics ((rl2RasterStatisticsPtr)
					     (lyr->raster_stats));
      }
    free (ptr->layers);
    free (ptr);
}

RL2_DECLARE rl2VariantArrayPtr
rl2_create_variant_array (int count)
{
/* creating an array of VariantValues */
    int i;
    rl2PrivVariantArrayPtr variant = malloc (sizeof (rl2PrivVariantArray));
    if (variant == NULL)
	return NULL;
    if (count < 1)
	return NULL;
    variant->count = count;
    variant->array = malloc (sizeof (rl2PrivVariantValuePtr) * count);
    if (variant->array == NULL)
      {
	  free (variant);
	  return NULL;
      }
    for (i = 0; i < variant->count; i++)
	*(variant->array + i) = NULL;
    return (rl2VariantArrayPtr) variant;
}

RL2_DECLARE void
rl2_destroy_variant_array (rl2VariantArrayPtr variant)
{
/* destroying an array of VariantValues */
    int i;
    rl2PrivVariantArrayPtr var = (rl2PrivVariantArrayPtr) variant;
    if (var == NULL)
	return;
    for (i = 0; i < var->count; i++)
      {
	  rl2PrivVariantValuePtr val = *(var->array + i);
	  if (val != NULL)
	      rl2_destroy_variant_value (val);
      }
    free (var->array);
    free (var);
}

RL2_DECLARE int
rl2_set_variant_int (rl2VariantArrayPtr variant, int index, const char *name,
		     sqlite3_int64 value)
{
/* setting an INTEGER VariantValue into a VariantArray object */
    rl2PrivVariantArrayPtr var = (rl2PrivVariantArrayPtr) variant;
    rl2PrivVariantValuePtr val;
    if (var == NULL)
	return RL2_ERROR;
    if (index >= 0 && index < var->count)
	;
    else
	return RL2_ERROR;
    val = malloc (sizeof (rl2PrivVariantValue));
    if (val == NULL)
	return RL2_ERROR;
    if (name == NULL)
	val->column_name = NULL;
    else
      {
	  int len = strlen (name);
	  val->column_name = malloc (len + 1);
	  strcpy (val->column_name, name);
      }
    val->int_value = value;
    val->text_value = NULL;
    val->blob_value = NULL;
    val->sqlite3_type = SQLITE_INTEGER;
    if (*(var->array + index) != NULL)
	rl2_destroy_variant_value (*(var->array + index));
    *(var->array + index) = val;
    return RL2_OK;
}

RL2_DECLARE int
rl2_set_variant_double (rl2VariantArrayPtr variant, int index,
			const char *name, double value)
{
/* setting a DOUBLE VariantValue into a VariantArray object */
    rl2PrivVariantArrayPtr var = (rl2PrivVariantArrayPtr) variant;
    rl2PrivVariantValuePtr val;
    if (var == NULL)
	return RL2_ERROR;
    if (index >= 0 && index < var->count)
	;
    else
	return RL2_ERROR;
    val = malloc (sizeof (rl2PrivVariantValue));
    if (val == NULL)
	return RL2_ERROR;
    if (name == NULL)
	val->column_name = NULL;
    else
      {
	  int len = strlen (name);
	  val->column_name = malloc (len + 1);
	  strcpy (val->column_name, name);
      }
    val->dbl_value = value;
    val->text_value = NULL;
    val->blob_value = NULL;
    val->sqlite3_type = SQLITE_FLOAT;
    if (*(var->array + index) != NULL)
	rl2_destroy_variant_value (*(var->array + index));
    *(var->array + index) = val;
    return RL2_OK;
}

RL2_DECLARE int
rl2_set_variant_text (rl2VariantArrayPtr variant, int index, const char *name,
		      const char *value, int bytes)
{
/* setting a TEXT VariantValue into a VariantArray object */
    rl2PrivVariantArrayPtr var = (rl2PrivVariantArrayPtr) variant;
    rl2PrivVariantValuePtr val;
    if (var == NULL)
	return RL2_ERROR;
    if (index >= 0 && index < var->count)
	;
    else
	return RL2_ERROR;
    val = malloc (sizeof (rl2PrivVariantValue));
    if (val == NULL)
	return RL2_ERROR;
    if (name == NULL)
	val->column_name = NULL;
    else
      {
	  int len = strlen (name);
	  val->column_name = malloc (len + 1);
	  strcpy (val->column_name, name);
      }
    val->text_value = malloc (bytes + 1);
    memcpy (val->text_value, value, bytes);
    *(val->text_value + bytes) = '\0';
    val->bytes = bytes;
    val->blob_value = NULL;
    val->sqlite3_type = SQLITE_TEXT;
    if (*(var->array + index) != NULL)
	rl2_destroy_variant_value (*(var->array + index));
    *(var->array + index) = val;
    return RL2_OK;
}

RL2_DECLARE int
rl2_set_variant_blob (rl2VariantArrayPtr variant, int index, const char *name,
		      const unsigned char *value, int bytes)
{
/* setting a BLOB VariantValue into a VariantArray object */
    rl2PrivVariantArrayPtr var = (rl2PrivVariantArrayPtr) variant;
    rl2PrivVariantValuePtr val;
    if (var == NULL)
	return RL2_ERROR;
    if (index >= 0 && index < var->count)
	;
    else
	return RL2_ERROR;
    val = malloc (sizeof (rl2PrivVariantValue));
    if (val == NULL)
	return RL2_ERROR;
    val->text_value = NULL;
    if (name == NULL)
	val->column_name = NULL;
    else
      {
	  int len = strlen (name);
	  val->column_name = malloc (len + 1);
	  strcpy (val->column_name, name);
      }
    val->blob_value = malloc (bytes);
    memcpy (val->blob_value, value, bytes);
    val->bytes = bytes;
    val->sqlite3_type = SQLITE_BLOB;
    if (*(var->array + index) != NULL)
	rl2_destroy_variant_value (*(var->array + index));
    *(var->array + index) = val;
    return RL2_OK;
}

RL2_DECLARE int
rl2_set_variant_null (rl2VariantArrayPtr variant, int index, const char *name)
{
/* setting a NULL VariantValue into a VariantArray object */
    rl2PrivVariantArrayPtr var = (rl2PrivVariantArrayPtr) variant;
    rl2PrivVariantValuePtr val;
    if (var == NULL)
	return RL2_ERROR;
    if (index >= 0 && index < var->count)
	;
    else
	return RL2_ERROR;
    val = malloc (sizeof (rl2PrivVariantValue));
    if (val == NULL)
	return RL2_ERROR;
    if (name == NULL)
	val->column_name = NULL;
    else
      {
	  int len = strlen (name);
	  val->column_name = malloc (len + 1);
	  strcpy (val->column_name, name);
      }
    val->text_value = NULL;
    val->blob_value = NULL;
    val->sqlite3_type = SQLITE_NULL;
    if (*(var->array + index) != NULL)
	rl2_destroy_variant_value (*(var->array + index));
    *(var->array + index) = val;
    return RL2_OK;
}

RL2_PRIVATE void
rl2_destroy_variant_value (rl2PrivVariantValuePtr value)
{
/* destoying a VariantValue object */
    if (value == NULL)
	return;
    if (value->column_name != NULL)
	free (value->column_name);
    if (value->text_value != NULL)
	free (value->text_value);
    if (value->blob_value != NULL)
	free (value->blob_value);
    free (value);
}
