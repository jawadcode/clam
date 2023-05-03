project(
  'clam',
  'c',
  default_options : [
    'warning_level=3',
    'c_std=c2x'
  ]
)

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : true)

add_project_arguments('-ftime-trace', language: 'c')

executable(
  'clam',
  sources : [
    'src/ast.c',
    'src/common.c',
    'src/lexer.c',
    'src/main.c',
    'src/memory.c',
    'src/parser.c'
  ],
  dependencies : m_dep
)