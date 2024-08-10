# SPDX-License-Identifier: GPL-2.0-or-later WITH Autoconf-exception-macro
#
#  Copyright (c) 2015 Lucas De Marchi <lucas.de.marchi@gmail.com>

# CC_FEATURE_APPEND([FLAGS], [ENV-TO-CHECK], [FLAG-NAME])
AC_DEFUN([CC_FEATURE_APPEND], [
  AS_VAR_PUSHDEF([FLAGS], [$1])dnl
  AS_VAR_PUSHDEF([ENV_CHECK], [$2])dnl
  AS_VAR_PUSHDEF([FLAG_NAME], [$3])dnl

  AS_CASE([" AS_VAR_GET(FLAGS) " ],
          [*" FLAG_NAME "*], [AC_RUN_LOG([: FLAGS already contains FLAG_NAME])],
          [
	    AS_IF([test "x$FLAGS" != "x"], [AS_VAR_APPEND(FLAGS, " ")])
            AS_IF([test "x$ENV_CHECK" = "xyes"],
                  [AS_VAR_APPEND(FLAGS, "+FLAG_NAME")],
                  [AS_VAR_APPEND(FLAGS, "-FLAG_NAME")])
          ]
  )

  AS_VAR_POPDEF([FLAG_NAME])dnl
  AS_VAR_POPDEF([ENV_CHECK])dnl
  AS_VAR_POPDEF([FLAGS])dnl
])
