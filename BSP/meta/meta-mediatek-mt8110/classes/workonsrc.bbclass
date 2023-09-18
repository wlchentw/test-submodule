# Copyright (C) 2015 Linux Foundation
# Author: Richard Sun
# Some code and influence taken from srctree.bbclass:
# Copyright (C) 2009 Chris Larson <clarson@kergoth.com>
# Released under the MIT license (see COPYING.MIT for the terms)
#
# workonsrc.bbclass enables use of an existing source tree, usually external to
# the build system to build a piece of software rather than the usual fetch/unpack/patch
# process.
#
# To use, add workonsrc to the global inherit and set WORKONSRC to point at the
# directory you want to use containing the sources e.g. from local.conf for a recipe
# called "myrecipe" you would do:
#
# INHERIT += "workonsrc"
# WORKONSRC_pn-myrecipe = "/path/to/my/source/tree"
#
# In order to make this class work for both target and native versions (or with
# multilibs/cross or other BBCLASSEXTEND variants), B is set to point to a separate
# directory under the work directory (split source and build directories). This is
# the default, but the build directory can be set to the source directory if
# circumstances dictate by setting WORKONSRC_BUILD to the same value, e.g.:
#
# WORKONSRC_BUILD_pn-myrecipe = "/path/to/my/source/tree"
#

SRCTREECOVEREDTASKS ?= "do_patch do_unpack do_fetch"

python () {
    import subprocess, os.path

    workonsrc = d.getVar('WORKONSRC', True)
    workonprebuilt = workonsrc.replace("build/../src/", "build/../prebuilt/")

    if os.path.exists(workonsrc):
        workonsrc = workonsrc
    elif os.path.exists(workonprebuilt):
        workonsrc = workonprebuilt
    else:
        bb.fatal("Both %s and %s aren't existed" % (workonsrc, workonprebuilt) )

    if workonsrc:
        d.setVar('S', workonsrc)
        workonsrcbuild = d.getVar('WORKONSRC_BUILD', True)
        if workonsrcbuild:
            d.setVar('B', workonsrcbuild)
        else:
            d.setVar('B', '${WORKDIR}/${BPN}-${PV}/')
            workonsrcbuild = d.getVar('B', True)

        if workonsrc != workonsrcbuild:
           cmd = "mkdir -p %s && rsync -aL %s/* %s" % (workonsrcbuild, workonsrc, workonsrcbuild)
           ret = subprocess.call(cmd, shell=True)
           if ret != 0:
               bb.error("mkdir -p %s && rsync -aL %s/* %s failed." % (workonsrcbuild, workonsrc, workonsrcbuild))
           d.setVar('S', workonsrcbuild)

        srcuri = (d.getVar('SRC_URI', True) or '').split()
        local_srcuri = []
        for uri in srcuri:
            if uri.startswith('file://'):
                local_srcuri.append(uri)
        d.setVar('SRC_URI', ' '.join(local_srcuri))

        if '{SRCPV}' in d.getVar('PV', False):
            # Dummy value because the default function can't be called with blank SRC_URI
            d.setVar('SRCPV', '999')

        tasks = filter(lambda k: d.getVarFlag(k, "task"), d.keys())

        for task in tasks:
            if task.endswith("_setscene"):
                # sstate is never going to work for external source trees, disable it
                bb.build.deltask(task, d)
            else:
                # Since configure will likely touch ${S}, ensure only we lock so one task has access at a time
                d.appendVarFlag(task, "lockfiles", "${S}/singletask.lock")

            # We do not want our source to be wiped out, ever (kernel.bbclass does this for do_clean)
            cleandirs = d.getVarFlag(task, 'cleandirs', False)
            if cleandirs:
                cleandirs = cleandirs.split()
                setvalue = False
                if '${S}' in cleandirs:
                    cleandirs.remove('${S}')
                    setvalue = True
                if workonsrcbuild == workonsrc and '${B}' in cleandirs:
                    cleandirs.remove('${B}')
                    setvalue = True
                if setvalue:
                    d.setVarFlag(task, 'cleandirs', ' '.join(cleandirs))

        fetch_tasks = ['do_fetch', 'do_unpack']
        # If we deltask do_patch, there's no dependency to ensure do_unpack gets run, so add one
        d.appendVarFlag('do_configure', 'deps', ['do_unpack'])

        for task in d.getVar("SRCTREECOVEREDTASKS", True).split():
            if local_srcuri and task in fetch_tasks:
                continue
            bb.build.deltask(task, d)

        d.prependVarFlag('do_compile', 'prefuncs', "workonsrc_compile_prefunc ")

        # Ensure compilation happens every time
        d.setVarFlag('do_compile', 'nostamp', '1')
}

python workonsrc_compile_prefunc() {
    # Make it obvious that this is happening, since forgetting about it could lead to much confusion
    bb.warn('Compiling %s from workon source %s' % (d.getVar('PN', True), d.getVar('WORKONSRC', True)))
}
