project(
  'clam',
  'c',
  default_options : [
    'warning_level=3',
    'c_std=c18'
  ]
)

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)

executable(
  'clam',
  sources : [
    'src/memory.c',
    'src/common.c',
    'src/lexer.c',
    'src/ast.c',
    'src/parser.c',
    'src/compiler.c',
    'src/vm.c',
    'src/main.c',
  ],
  dependencies : m_dep
)
