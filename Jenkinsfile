// Pipeline definition for the build steps of AR.
//
// Copyright: 2017 Ditto Technologies. All Rights Reserved.
// Author: Frankie Li, Daran He, John Inacay
// TODO - Need to migrate to a standardize Debian package deployment script.


build_configs = [
    'ubuntu_16_04' : [
        'docker_name' : 'ubuntu16-build-env',
        'docker_file' : 'Dockerfile.xenial',
        'repo'        : '3rdparty-16.04',
        'dist'        : 'xenial',
    ],
    'ubuntu_14_04' : [
        'docker_name' : 'ubuntu14-build-env',
        'docker_file' : 'Dockerfile.trusty',
        'repo'        : '3rdparty-14.04',
        'dist'        : 'trusty',
    ]
]


node('build && docker') {
    // Set max number of builds to keep to 5.
    properties([[$class: 'BuildDiscarderProperty', strategy: [$class: 'LogRotator', artifactDaysToKeepStr: '', artifactNumToKeepStr: '', daysToKeepStr: '', numToKeepStr: '5']]]);

    build_configs.each { target, build_config ->
        is_rc = false
        is_release = false

        checkoutRepo(target)

        build(target, build_config)

        if (is_release) {
            publish(target, build_config)
        }

        cleanUp(target)
    }
}

def checkoutRepo(target) {
    stage("Checkout ${target}") {
        // Pull the code from the repo. `checkout` is a special Jenkins cmd.
        def scm_vars = checkout scm
        git_tag = getGitTag()
        git_branch = getGitBranch()

        if(git_tag) {
            is_rc = (git_tag.indexOf("-rc") >= 0)
            is_release = (git_tag.indexOf("release") >= 0)
        }
        echo "Current branch is: ${git_branch}, current tag is: ${git_tag}"
        echo "Current tag is a RC tag: ${is_rc}"
        echo "Current tag is a Release tag: ${is_release}"
    }
}

def build(target, build_config) {
    stage("Prepare Build Env ${target}") {
        // We're checking to see if an old image exists. If so, delete it to
        // reduce total space usage.
        def docker_name = build_config.docker_name
        def docker_file = build_config.docker_file


        def old_image = sh (script: "docker images -q ${build_config.docker_name}",
                            returnStdout: true).replace("\n", " ")
        echo "Old docker image: ${old_image}"

        docker.build("${build_config.docker_name}", "-f ${build_config.docker_file} .")

        def new_image = sh (script: "docker images -q ${build_config.docker_name}",
                            returnStdout: true).replace("\n", " ")

        echo "New docker image: ${new_image}"

        if (old_image.length() > 0 && old_image != new_image) {
            def children = sh(script: "docker images --filter 'dangling=true' -q --no-trunc",
                              returnStdout: true).replace("\n", " ")
            echo "Removing children: ${children}"
            sh("docker rmi ${children}")
            echo "Removing old image: ${old_image}"
            sh("docker rmi ${old_image}")
        }
    }

    stage("Build ${target}") {
        def USER_ID = sh (
            script: 'id -u',
            returnStdout: true
        ).trim()
        def GROUP_ID = sh (
            script: 'id -g',
            returnStdout: true
        ).trim()

        withEnv(['USER_ID=${USER_ID}','GROUP_ID=${GROUP_ID}',
                 'RELEASE_KEYSTORE=keystore.jks',
                 'RELEASE_KEY_ALIAS=demoapp',
                 'RELEASE_STORE_PASSWORD=ditto1',
                 'RELEASE_KEY_PASSWORD=ditto1']) {
            docker.image("${build_config.docker_name}").inside {
                sh('./run_build.sh')
            }
        }
    }

    stage("ArchiveArtifacts ${target}") {
        archiveArtifacts(artifacts: 'build/*.deb')
    }
}

def publish(target, build_config) {
    stage("Publish ${target}") {
        withAWS(credentials:'package-uploads') {
            sh("./publish.sh ${build_config.repo} ${build_config.dist}")
        }
    }
}

def cleanUp(target) {
    stage("CleanUp ${target}") {
        def current_dir = pwd()
        echo "Cleaning up ${current_dir}"
        deleteDir()
    }
}

def getGitHash() {
    return sh(script: "git log -n1 --pretty='%h'", returnStdout: true).trim()
}

def getGitTag() {
    def hash = getGitHash()
    def tag = ""
    try {
        tag = sh(script: "git describe --exact-match --tags ${hash}", returnStdout: true).trim()
    } catch (e) {
      echo "No current tag."
    }
    return tag
}

def getGitBranch() {
    return sh(script: "git rev-parse --abbrev-ref HEAD", returnStdout: true).trim()
}