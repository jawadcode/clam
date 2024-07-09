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
    'src/ast.c',
    'src/compiler.c',
    'src/lexer.c',
    'src/memory.c',
    'src/parser.c',
    'src/string.c',
    'src/vm.c',

    'src/main.c'
  ],
  dependencies : m_dep
)
