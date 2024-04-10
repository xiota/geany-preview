# tristate_option
#
#   Creates cache variable to store an option.
#   Provides access variables for convenience.
#
#   Inputs:
#     VAR_NAME        = Variable to access option
#     VAR_DEFAULT     = Default option, AUTO/ON/OFF
#     VAR_DESCRIPTION = Option description
#
#   Outputs:
#     VAR_NAME = The cache variable itself
#     VAR_NAME_AUTO = 1 if VAR_NAME is AUTO, 0 if ON/OFF
#     VAR_NAME_ON   = 1 if VAR_NAME is ON, 0 if AUTO/OFF
#     VAR_NAME_OFF  = 1 if VAR_NAME is OFF, 0 if AUTO/ON
#     VAR_NAME_REQUIRED = "REQUIRED" if VAR_NAME is ON, "" if AUTO/OFF
#
#   Example:
#     tristate_option(MY_VAR AUTO "Example tristate variable")
#
macro(tristate_option VAR_NAME VAR_DEFAULT VAR_DESCRIPTION)
  set(${VAR_NAME} ${VAR_DEFAULT} CACHE STRING "${VAR_DESCRIPTION}")
  set_property(CACHE ${VAR_NAME} PROPERTY STRINGS AUTO ON OFF)

  set (${VAR_NAME}_REQUIRED "")
  if (${VAR_NAME} AND NOT ${VAR_NAME} STREQUAL "AUTO")
    set(${VAR_NAME}_ON 1) # ON
    set (${VAR_NAME}_REQUIRED "REQUIRED")
  else()
    set(${VAR_NAME}_ON 0) # OFF OR AUTO
  endif()

  if (NOT ${VAR_NAME} AND NOT ${VAR_NAME} STREQUAL "AUTO")
    set(${VAR_NAME}_OFF 1) # OFF
  else()
    set(${VAR_NAME}_OFF 0) # ON OR AUTO
  endif()

  if (${VAR_NAME} STREQUAL "AUTO")
    set(${VAR_NAME}_AUTO 1) # AUTO
  else()
    set(${VAR_NAME}_AUTO 0) # ON OR OFF
  endif()
endmacro()
