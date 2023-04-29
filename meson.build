project('clam', 'c')

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : true)

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