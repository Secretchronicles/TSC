#!/usr/bin/env python3
#
# update-download-pages.py <tag> <website-root>
#
# Rewrite the "Beta Version" download tables on the TSC website
# (Secretchronicles/secretchronicles.github.io) so they point at ONE release -
# the tag this is given - with that release's real download links and its real
# md5/sha256 checksums. It edits en/es/fi/download/index.html in place.
#
# WHY IT LIVES HERE. The packages are built here, in Secretchronicles/TSC, by
# Release all / Release all missing; the page that offers them to download lives
# in the other repository. Keeping the two in step by hand is how the page came
# to still advertise v2.2.0-beta1 long after beta2 existed. So the workflow that
# builds a release also rewrites the page for it: Repos.yml runs this, then
# commits the result to the website (see that workflow).
#
# WHERE THE DATA COMES FROM. Nothing is hardcoded about which packages a release
# has. `gh release view` lists the assets, `gh release download` fetches only the
# small *.md5sum / *.sha256sum text files (never the binaries), and the tables
# are built from whatever is actually there - so a release with no macOS image
# simply has no macOS table, and the day one gains a new architecture it appears
# on its own. Checksums are named <asset>.md5sum / <asset>.sha256sum (the full
# asset name kept), which is what lets an AppImage and the same-arch Flatpak each
# keep their own; an asset whose checksum is not published gets an empty cell
# rather than a wrong one.
#
# WHERE IT WRITES. Between the markers
#   <!-- TSC-DOWNLOADS:START --> ... <!-- TSC-DOWNLOADS:END -->
# in each language's download page. The first time, before the markers exist, it
# replaces the old hand-written "Beta Version" heading and paragraph (from
# <h2 id="beta-version"> up to the next <h2>) and leaves the markers behind, so
# every run after that is a clean marker-to-marker replacement.
#
# The repository gh talks to is $GH_REPO if set (the workflow sets it to
# github.repository), else Secretchronicles/TSC. GH_TOKEN must be in the
# environment, as it already is in Actions.

import os, re, sys, json, subprocess, tempfile, html

REPO = os.environ.get("GH_REPO", "Secretchronicles/TSC")
LANGS = ("en", "es", "fi")
START = "<!-- TSC-DOWNLOADS:START -->"
END = "<!-- TSC-DOWNLOADS:END -->"

# CPU labels, kept technical (the existing tables leave these in English).
CPU = {
    "amd64": "amd64 (64-bit x86)", "arm64": "arm64 (64-bit ARM)",
    "armhf": "armhf (32-bit ARM)", "ppc64el": "ppc64el (PowerPC)",
    "riscv64": "riscv64 (RISC-V)", "s390x": "s390x (IBM Z)",
    "i386": "i386 (32-bit x86)", "x86_64": "64-bit x86", "aarch64": "64-bit ARM",
    "win64": "64-bit x86", "win32": "32-bit x86", "windows": "32/64-bit x86",
    "all": "all",
}

# Debian/Ubuntu distribution code name -> shown platform name.
DISTRO = {
    "resolute": "Ubuntu 26.04 Resolute Raccoon",
    "forky": "Debian 14 Forky",
    "noble": "Ubuntu 26.04 Noble Numbat",
    "jammy": "Ubuntu 22.04 Jammy Jellyfish",
    "bookworm": "Debian 12 Bookworm",
    "bullseye": "Debian 11 Bullseye",
    "trixie": "Debian 13 Trixie",
    "sid": "Debian Sid Unstable",
}

L = {
    "en": dict(
        h2="Beta Version", summary="Checksums", md5="MD5 sum:", sha="SHA256 hash:",
        ver="Version", plat="Platform", cpu="CPU", dl="Download", chk="Checksums",
        intro='{v} was released on {date}. Its packages are attached to the '
              '<a href="{tag_url}">GitHub release</a> and listed below.',
        intro_nodate='{v} packages are attached to the '
              '<a href="{tag_url}">GitHub release</a> and listed below.',
        note='Each Debian and Ubuntu <code>.deb</code> below also needs the shared '
             '<code>{data}</code> data package installed alongside it.',
        p_linux="Any Linux distribution", p_flatpak="Any distribution with Flatpak",
        p_win="Windows (64-bit)", p_win32="Windows (32-bit)", p_data="Shared data package",
        h_appimage="AppImage", h_flatpak="Flatpak", h_win="Windows", h_mac="macOS",
        h_data="Shared data package",
    ),
    "es": dict(
        h2="Versión Beta", summary="Sumas de verificación", md5="Suma MD5:", sha="Hash SHA256:",
        ver="Versión", plat="Plataforma", cpu="CPU", dl="Descarga", chk="Sumas de verificación",
        intro='{v} se publicó el {date}. Sus paquetes están adjuntos a la '
              '<a href="{tag_url}">versión de GitHub</a> y se enumeran a continuación.',
        intro_nodate='Los paquetes de {v} están adjuntos a la '
              '<a href="{tag_url}">versión de GitHub</a> y se enumeran a continuación.',
        note='Cada <code>.deb</code> de Debian y Ubuntu de abajo necesita además que se '
             'instale junto a él el paquete de datos compartido <code>{data}</code>.',
        p_linux="Cualquier distribución de Linux", p_flatpak="Cualquier distribución con Flatpak",
        p_win="Windows (64 bits)", p_win32="Windows (32 bits)", p_data="Paquete de datos compartido",
        h_appimage="AppImage", h_flatpak="Flatpak", h_win="Windows", h_mac="macOS",
        h_data="Paquete de datos compartido",
    ),
    "fi": dict(
        h2="Beta Versio", summary="Tarkistussummat", md5="MD5 summa:", sha="SHA256 tiiviste:",
        ver="Versio", plat="Alusta", cpu="Suoritin", dl="Lataa", chk="Tarkistussummat",
        intro='{v} julkaistiin {date}. Sen paketit on liitetty '
              '<a href="{tag_url}">GitHub-julkaisuun</a> ja luetellaan alla.',
        intro_nodate='{v} paketit on liitetty '
              '<a href="{tag_url}">GitHub-julkaisuun</a> ja luetellaan alla.',
        note='Jokainen alla oleva Debianin ja Ubuntun <code>.deb</code> tarvitsee lisäksi '
             'rinnalleen asennettuna jaetun datapaketin <code>{data}</code>.',
        p_linux="Mikä tahansa Linux-jakelu", p_flatpak="Mikä tahansa jakelu, jossa on Flatpak",
        p_win="Windows (64-bittinen)", p_win32="Windows (32-bittinen)", p_data="Jaettu datapaketti",
        h_appimage="AppImage", h_flatpak="Flatpak", h_win="Windows", h_mac="macOS",
        h_data="Jaettu datapaketti",
    ),
}


def gh_json(tag):
    out = subprocess.run(
        ["gh", "release", "view", tag, "--repo", REPO,
         "--json", "assets,publishedAt,createdAt,tagName"],
        check=True, capture_output=True, text=True).stdout
    return json.loads(out)


def download_sums(tag, into):
    # Only the small checksum text files, never the binaries.
    subprocess.run(
        ["gh", "release", "download", tag, "--repo", REPO,
         "--pattern", "*.md5sum", "--pattern", "*.sha256sum",
         "--dir", into, "--clobber"],
        check=False, capture_output=True, text=True)
    md5, sha = {}, {}
    for fn in os.listdir(into):
        path = os.path.join(into, fn)
        try:
            line = open(path, encoding="utf-8").read().strip().split("\n")[0]
        except OSError:
            continue
        parts = line.split()
        if len(parts) < 2:
            continue
        digest = parts[0]
        target = parts[-1].lstrip("*")  # md5sum writes " *name" for binary mode
        if fn.endswith(".md5sum"):
            md5[target] = digest
        elif fn.endswith(".sha256sum"):
            sha[target] = digest
    return md5, sha


def classify(name, version):
    # Returns (category, cpu_key, sort_index) or None to skip.
    stem = name
    prefix = "TSC-%s-" % version
    if not name.startswith(prefix):
        # build-script zips, config, and anything unversioned: not a download.
        return None
    rest = name[len(prefix):]
    if rest.endswith(".AppImage"):
        return ("appimage", rest[:-len(".AppImage")])
    if rest.endswith(".flatpak"):
        return ("flatpak", rest[:-len(".flatpak")])
    if rest == "data-all.deb":
        return ("data", "all")
    if rest.endswith(".deb"):
        return ("deb", rest[:-len(".deb")])  # <distro>-<arch>
    if rest.endswith(".exe") or rest.endswith(".7z"):
        return ("windows", rest)  # keep win64.exe / win64.7z whole
    if rest.endswith(".dmg"):
        return ("mac", rest[:-len(".dmg")])
    return None


def checksum_cell(t, name, md5, sha):
    m, s = md5.get(name), sha.get(name)
    if not m and not s:
        return "    <td>&nbsp;</td>"
    body = []
    if m:
        body.append("    %s<br /><code>%s</code>" % (t["md5"], m))
    if s:
        body.append("    %s<br /><code>%s</code>" % (t["sha"], s))
    return ("    <td>\n    <details>\n    <summary>%s</summary>\n\n%s\n    </details>\n  </td>"
            % (t["summary"], "<br />\n".join(body)))


def row(t, base_url, version, platform, cpu, name, md5, sha):
    return ("  <tr>\n"
            "    <td>%s</td>\n"
            "    <td>%s</td>\n"
            "    <td>%s</td>\n"
            "    <td><a href=\"%s%s\">%s</a></td>\n"
            "%s\n"
            "  </tr>" % (version, platform, cpu, base_url, name, name,
                         checksum_cell(t, name, md5, sha)))


def table(t, rows):
    head = ("  <tr><th>%s</th><th>%s</th><th>%s</th><th>%s</th><th>%s</th></tr>"
            % (t["ver"], t["plat"], t["cpu"], t["dl"], t["chk"]))
    return '<table class="downloads">\n' + head + "\n" + "\n".join(rows) + "\n</table>"


# One order that reads naturally in BOTH an AppImage table (x86_64, aarch64,
# armhf) and a .deb table (amd64, arm64, armhf, ...): 64-bit x86 first, then
# 64-bit ARM, then 32-bit ARM, then the less common ones. The two naming schemes
# never share a table, so interleaving x86_64/amd64 and aarch64/arm64 is fine.
ARCH_ORDER = ["x86_64", "amd64", "aarch64", "arm64", "armhf", "ppc64el",
              "riscv64", "s390x", "i386"]


def arch_key(a):
    return ARCH_ORDER.index(a) if a in ARCH_ORDER else len(ARCH_ORDER)


# The AppImage that runs on glibc 2.35 and newer is TSC-<version>-<arch>.AppImage; the same file is
# also published as TSC-<version>-raspberrypios-<arch>.AppImage (AppImage.yml).
APPIMAGE_DISTROS = [("raspberrypios", "Raspberry Pi OS")]


def split_appimage(key):
    # "ubuntu2610-aarch64" -> ("ubuntu2610", "aarch64"); "x86_64" -> ("", "x86_64")
    for distro, _ in APPIMAGE_DISTROS:
        if key.startswith(distro + "-"):
            return distro, key[len(distro) + 1:]
    return "", key


def appimage_sort(key):
    distro, arch = split_appimage(key)
    names = [d for d, _ in APPIMAGE_DISTROS]
    # the generic builds first, then by distribution, then 64-bit x86, 64-bit ARM, 32-bit ARM
    return (names.index(distro) + 1 if distro in names else 0, arch_key(arch))


def cpu_label(key):
    distro, arch = split_appimage(key)
    label = CPU.get(arch, arch)
    names = dict(APPIMAGE_DISTROS)
    return "%s - %s" % (names[distro], label) if distro in names else label


def build_section(lang, version, tag, tag_url, date, assets, md5, sha, base_url):
    t = L[lang]
    buckets = {"appimage": [], "flatpak": [], "windows": [], "mac": [], "data": []}
    debs = {}  # distro -> [(arch, name)]
    for a in assets:
        c = classify(a, version)
        if not c:
            continue
        cat, keypart = c
        if cat == "deb":
            distro, _, arch = keypart.partition("-")
            debs.setdefault(distro, []).append((arch, a))
        else:
            buckets[cat].append((keypart, a))

    out = [START, ""]
    out.append('<h2 id="beta-version">%s</h2>' % t["h2"])
    out.append("")
    data_name = next((a for (_, a) in buckets["data"]), "TSC-%s-data-all.deb" % version)
    if date:
        intro = t["intro"].format(v=version, date=date, tag_url=tag_url)
    else:
        intro = t["intro_nodate"].format(v=version, tag_url=tag_url)
    out.append("<p>%s</p>" % intro)
    out.append("")
    if debs:
        out.append("<p>%s</p>" % t["note"].format(data=html.escape(data_name)))
        out.append("")

    def add_table(anchor, heading, rows):
        out.append('<h3 id="%s">%s</h3>' % (anchor, heading))
        out.append("")
        out.append(table(t, rows))
        out.append("")

    # AppImage
    if buckets["appimage"]:
        rows = [row(t, base_url, version, t["p_linux"], cpu_label(k), n, md5, sha)
                for (k, n) in sorted(buckets["appimage"], key=lambda kn: appimage_sort(kn[0]))]
        add_table("beta-appimage", t["h_appimage"], rows)
    # Flatpak
    if buckets["flatpak"]:
        rows = [row(t, base_url, version, t["p_flatpak"], cpu_label(k), n, md5, sha)
                for (k, n) in sorted(buckets["flatpak"], key=lambda kn: arch_key(kn[0]))]
        add_table("beta-flatpak", t["h_flatpak"], rows)
    # Windows
    if buckets["windows"]:
        def win_plat(n):
            return t["p_win32"] if "win32" in n else t["p_win"]
        def win_cpu(n):
            return cpu_label("win32" if "win32" in n else ("windows" if "-windows." in ("-" + n) else "win64"))
        # The .exe installer before the .7z archive - it is what most people want.
        def win_sort(n):
            return (0 if n.endswith(".exe") else 1 if n.endswith(".7z") else 2, n)
        rows = [row(t, base_url, version, win_plat(n), win_cpu(n), n, md5, sha)
                for (_, n) in sorted(buckets["windows"], key=lambda kn: win_sort(kn[1]))]
        add_table("beta-windows", t["h_win"], rows)
    # Debian/Ubuntu distributions
    order = ["resolute", "noble", "jammy", "forky", "trixie", "bookworm", "bullseye", "sid"]
    seen = [d for d in order if d in debs] + [d for d in debs if d not in order]
    for i, distro in enumerate(seen):
        plat = DISTRO.get(distro, distro.capitalize())
        rows = [row(t, base_url, version, plat, cpu_label(arch), n, md5, sha)
                for (arch, n) in sorted(debs[distro], key=lambda an: arch_key(an[0]))]
        anchor = "beta-deb-%s" % re.sub(r"[^a-z0-9]+", "-", distro.lower())
        add_table(anchor, plat, rows)
    # macOS
    if buckets["mac"]:
        rows = [row(t, base_url, version, "macOS", n.replace("-", " "), fn, md5, sha)
                for (n, fn) in sorted(buckets["mac"], key=lambda kn: kn[0])]
        add_table("beta-mac", t["h_mac"], rows)
    # Shared data package
    if buckets["data"]:
        rows = [row(t, base_url, version, t["p_data"], "all", n, md5, sha)
                for (_, n) in buckets["data"]]
        add_table("beta-data", t["h_data"], rows)

    if out and out[-1] == "":
        out.pop()
    out.append(END)
    return "\n".join(out)


def splice(htmltext, section):
    if START in htmltext and END in htmltext:
        return re.sub(re.escape(START) + r".*?" + re.escape(END), lambda m: section,
                      htmltext, count=1, flags=re.DOTALL)
    # First run: replace the old hand-written beta block (from its <h2> to the
    # next <h2>) and leave the markers behind.
    m = re.search(r'<h2 id="beta-version">', htmltext)
    if not m:
        raise SystemExit("no <h2 id=\"beta-version\"> and no markers to anchor on")
    nxt = re.search(r'\n<h2\b', htmltext[m.end():])
    if not nxt:
        raise SystemExit("could not find the heading after the beta section")
    end_idx = m.end() + nxt.start()
    return htmltext[:m.start()] + section + htmltext[end_idx:]


def main():
    if len(sys.argv) != 3:
        sys.exit("usage: update-download-pages.py <tag> <website-root>")
    tag, root = sys.argv[1], sys.argv[2]
    version = tag[1:] if tag.startswith("v") else tag
    tag_url = "https://github.com/%s/releases/tag/%s" % (REPO, tag)
    base_url = "https://github.com/%s/releases/download/%s/" % (REPO, tag)

    meta = gh_json(tag)
    assets = [a["name"] for a in meta.get("assets", [])]
    stamp = meta.get("publishedAt") or meta.get("createdAt") or ""
    date = stamp[:10] if stamp else ""

    with tempfile.TemporaryDirectory() as tmp:
        md5, sha = download_sums(tag, tmp)

    changed = []
    for lang in LANGS:
        path = os.path.join(root, lang, "download", "index.html")
        if not os.path.exists(path):
            print("skip (missing): %s" % path)
            continue
        text = open(path, encoding="utf-8").read()
        section = build_section(lang, version, tag, tag_url, date, assets, md5, sha, base_url)
        new = splice(text, section)
        if new != text:
            open(path, "w", encoding="utf-8").write(new)
            changed.append(path)
            print("updated %s" % path)
        else:
            print("unchanged %s" % path)
    print("Done: %d file(s) changed for %s." % (len(changed), tag))


if __name__ == "__main__":
    main()
