# need to inherit this bbclass after core-image
def staging_copyfile(c, target, dest, postinsts, seendirs):
    import errno

    destdir = os.path.dirname(dest)
    if destdir not in seendirs:
        bb.utils.mkdirhier(destdir)
        seendirs.add(destdir)
    if "/usr/bin/postinst-" in c:
        postinsts.append(dest)
    if os.path.islink(c):
        linkto = os.readlink(c)
        if os.path.lexists(dest):
            if not os.path.islink(dest):
                raise OSError(errno.EEXIST, "Link %s already exists as a file" % dest, dest)
            if os.readlink(dest) == linkto:
                return dest
            raise OSError(errno.EEXIST, "Link %s already exists to a different location? (%s vs %s)" % (dest, os.readlink(dest), linkto), dest)
        os.symlink(linkto, dest)
        #bb.warn(c)
    else:
        try:
            os.link(c, dest)
        except OSError as err:
            if err.errno == errno.EXDEV or err.errno == errno.EEXIST:
                bb.utils.copyfile(c, dest)
            else:
                raise
    return dest
