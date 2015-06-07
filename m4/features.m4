#  Copyright (c) 2015 Lucas De Marchi <lucas.de.marchi@gmail.com>
#
#  This program is free software: you can redistribute it and/or modify it
#  under the terms of the GNU General Public License as published by the
#  Free Software Foundation, either version 2 of the License, or (at your
#  option) any later version.
#
#  This program is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
#  Public License for more details.
#
#  You should have received a copy of the GNU General Public License along
#  with this program. If not, see <http://www.gnu.org/licenses/>.
#
#  As a special exception, the respective Autoconf Macro's copyright owner
#  gives unlimited permission to copy, distribute and modify the configure
#  scripts that are the output of Autoconf when processing the Macro. You
#  need not follow the terms of the GNU General Public License when using
#  or distributing such scripts, even though portions of the text of the
#  Macro appear in them. The GNU General Public License (GPL) does govern
#  all other use of the material that constitutes the Autoconf Macro.
#
#  This special exception to the GPL applies to versions of the Autoconf
#  Macro released by the Autoconf Archive. When you make and distribute a
#  modified version of the Autoconf Macro, you may extend this special
#  exception to the GPL to apply to your modified version as well.
#
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
