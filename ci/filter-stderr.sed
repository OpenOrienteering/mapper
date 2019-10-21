# Cf. https://github.com/Microsoft/azure-pipelines-tasks/blob/master/docs/authoring/commands.md
/^\/.*openorienteering.*[Ww]arning:/ {
  s,^\([a-zA-Z]:\)?/.*/build/source/openorienteering-mapper-ci/,,
  s,^\([a-zA-Z]:\)?/.*/build/,,
  s,^,##vso[task.LogIssue type=warning;],
  s,^\([^;]*\);\]\([^:]*\):\([0-9]*\):\([0-9]*\): *[Ww]arning: *,\1;sourcepath=\2;linenumber=\3;columnnumber=\4;],
}
/^\/.*[Ee]rror:/ {
  s,^\([a-zA-Z]:\)?/.*/build/source/openorienteering-mapper-ci/,,
  s,^\([a-zA-Z]:\)?/.*/build/,,
  s,^,##vso[task.LogIssue type=error;],
  s,^\([^;]*\);\]\([^:]*\):\([0-9]*\):\([0-9]*\): *[Ee]rror: *,\1;sourcepath=\2;linenumber=\3;columnnumber=\4;],
}
# Include-what-you-use
/ should add these lines:$/ {
  h
  s,^\([a-zA-Z]:\)?/.*/build/source/openorienteering-mapper-ci/,,
  s,^\([a-zA-Z]:\)?/.*/build/,,
  s,^,##vso[task.LogIssue type=warning;],
  s,^\([^;]*\);\]\([^:]*\) should add these lines:,\1;sourcepath=\2;]include-what-you-use reported diagnostics,
  p
  g
}
