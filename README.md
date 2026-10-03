# aMule Keroro

<!-- Français -->

## Français

Fork expérimental d’aMule qui coordonne plusieurs instances pour télécharger
un fichier partie par partie et relayer les parties vérifiées entre les nœuds.
Il conserve le client eD2k/Kad d’aMule et ajoute des commandes relais pour
l’interface graphique et le démon sans interface `amuled`.

### Mode relais

Chaque nœud utilise le même hash de fichier, le UserHash eD2k de la source S et
une base de pseudonyme partagée. Le numéro du nœud indique sa place dans la
séquence. Les nœuds découvrent leurs voisins à partir de leur pseudonyme,
récupèrent les parties vérifiées nécessaires et les transmettent via eD2k. Le
hash de S limite à cette source les parties qui lui sont attribuées.

Par défaut, chaque numéro de séquence correspond à la partie de fichier portant
le même numéro. L’option « final-first » télécharge d’abord la dernière partie,
puis les parties `0, 1, 2, ...`. Le mode ping-pong à deux nœuds alterne le
travail entre des démons dont les pseudonymes se terminent par `-0` et `-1`. Une
partie est relayée après sa réception et sa vérification locales.

Dans l’interface graphique, configurez le relais dans la boîte de dialogue
**Force Part**. Sélectionnez une partie, choisissez une source connue ou
saisissez le UserHash de S, indiquez la base de pseudonyme et cochez les options
souhaitées.

Pour lancer une séquence sans interface, utilisez `amuled` :

```sh
amuled --force-part-sequence-chain \
  '<hash-ed2k-du-fichier>:<étape>:<userhash-de-S>:<base-de-pseudonyme>'
```

Ajoutez `--force-part-sequence-pingpong` sur les deux démons pour le mode à deux
nœuds ; le premier démarre à l’étape `0`, le second à l’étape `1`. Ajoutez
`--force-part-sequence-final-first` sur chaque nœud pour télécharger la dernière
partie en premier. Tous les nœuds doivent utiliser des versions compatibles et
les mêmes paramètres de séquence.

### Compilation

Le projet utilise CMake. Sous Linux, installez les dépendances de compilation
d’aMule, puis lancez :

```sh
cmake -B build -DBUILD_MONOLITHIC=YES -DBUILD_REMOTEGUI=YES
cmake --build build -j"$(nproc)"
```

Pour compiler sous Windows, utilisez l’environnement MSYS2 `CLANG64` et les
scripts de packaging Windows du projet. Après toute modification du protocole
relais, recompilez chaque nœud avant de tester un transfert.

### État du projet

Il s’agit d’un fork expérimental propre à ce projet. Le relais a été essayé
entre des démons Linux et Windows, mais toute nouvelle modification du protocole
doit être validée avec les versions exactes déployées sur les nœuds.

---

<!-- English -->

## English

An experimental aMule fork that coordinates multiple aMule instances to
download a file part by part and relay verified parts between nodes. It keeps
aMule's eD2k/Kad client and adds relay controls for both the GUI and the
headless `amuled` daemon.

### Relay mode

Each node is configured with the same file hash, origin S's eD2k UserHash, and
shared nickname base. The node number identifies its place in the sequence.
Nodes discover adjacent relays by nickname, fetch the required verified parts,
and pass them on using eD2k transfers. The origin hash restricts the source
used for parts assigned to S.

The default order maps each sequence number to the corresponding file part.
Optional final-first order downloads the last part first, followed by parts
`0, 1, 2, ...`. Two-node ping-pong mode alternates relay work between daemons
named with the `-0` and `-1` suffixes. A part is relayed after it has been
received and verified locally.

In the GUI, configure this in the **Force Part** dialog. Select a file part,
choose a known source or enter S's UserHash, set the shared nickname base, and
select the desired relay options.

For headless operation, start a sequence with `amuled`:

```sh
amuled --force-part-sequence-chain \
  '<file-ed2k-hash>:<sequence-step>:<origin-user-hash>:<nickname-base>'
```

Add `--force-part-sequence-pingpong` on both daemons for two-node operation;
the first daemon starts at step `0`, the second at step `1`. Add
`--force-part-sequence-final-first` on every node to use the final-first order.
All nodes must use compatible builds and the same sequence settings.

### Build

The project uses CMake. On Linux, install the required aMule build dependencies,
then run:

```sh
cmake -B build -DBUILD_MONOLITHIC=YES -DBUILD_REMOTEGUI=YES
cmake --build build -j"$(nproc)"
```

For a Windows build, use the MSYS2 `CLANG64` environment and the project's
Windows packaging scripts. Relay protocol changes should be rebuilt on every
participating node before testing a transfer.

### Status

This is a project-specific experimental fork. The relay workflow has been
exercised between Linux and Windows daemons, but each new protocol change still
needs validation with the exact builds deployed on all nodes.
