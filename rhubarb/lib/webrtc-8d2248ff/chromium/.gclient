solutions = [{
  'name': 'src',
  'url': 'https://chromium.googlesource.com/chromium/src.git',
  'deps_file': '.DEPS.git',
  'managed': False,
  'custom_deps': {
    # Skip syncing some large dependencies WebRTC will never need.
    'src/chrome/tools/test/reference_build/chrome_linux': None,
    'src/chrome/tools/test/reference_build/chrome_mac': None,
    'src/chrome/tools/test/reference_build/chrome_win': None,
    'src/native_client': None,
    'src/third_party/cld_2/src': None,
    'src/third_party/hunspell_dictionaries': None,
    'src/third_party/liblouis/src': None,
    'src/third_party/pdfium': None,
    'src/third_party/skia': None,
    'src/third_party/trace-viewer': None,
    'src/third_party/webrtc': None,
  },
  'safesync_url': ''
}]

cache_dir = None
