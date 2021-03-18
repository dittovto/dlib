// Pipeline definition for the build steps of AR.
//
// Copyright: 2017 Ditto Technologies. All Rights Reserved.
// Author: Frankie Li, Daran He, John Inacay, Oleg Selivanov

// TODO: Need to migrate to a standardize Debian package deployment script.

properties([[
  $class: 'BuildDiscarderProperty',
  strategy: [
    $class: 'LogRotator',
    artifactDaysToKeepStr: '',
    artifactNumToKeepStr: '',
    daysToKeepStr: '',
    numToKeepStr: '5'
  ]
]]);

@Library('jenkins-shared-library@ubuntu-20.04') _

def GIT_CREDENTIALS_ID = 'dittovto-buildbot'

def BUILD_CONFIGS = [
    'ubuntu-16-04' : [
        'docker_file'   : 'Dockerfile.xenial',
        'apt_prod_repo' : '3rdparty-16.04',
        'apt_test_repo' : '3rdparty-16.04-staging',
        'dist'          : 'xenial',
        'nexus_prod'    : 'ditto-xenial-release',
        'nexus_test'    : 'ditto-xenial-testing',
    ],
    'ubuntu-14-04' : [
        'docker_file'   : 'Dockerfile.trusty',
        'apt_prod_repo' : '3rdparty-14.04',
        'apt_test_repo' : '3rdparty-14.04-staging',
        'dist'          : 'trusty',
        'nexus_prod'    : '',
        'nexus_test'    : '',
    ]
]

def FIRST_PLATFORM = (BUILD_CONFIGS.keySet() as List).sort()[0]

node('static-minion') {
  BUILD_CONFIGS.each { platform, build_config ->
    dir(platform) {
      stage("Checking out ${platform}") {
        git_info = ditto_git.checkoutRepo()

        ditto_config = ditto_utils.readDittoConfig()
        version = ditto_config.version
        origin = ditto_config.origin

        if (git_info.is_release_branch) {
          ditto_utils.checkVersionInReleaseBranchName(git_info.branch, version)
        }
      }

      stage("Copying over debian packaging resources ${platform}") {
        ditto_deb.copyDebianPipelineResourceFilesHere()
      }

      stage("Building and publishing ${platform} dev revision") {
        revision =
          ditto_deb.buildDevRevisionString(origin, version, git_info.commit)
        image_name =
          ditto_utils.buildDockerImageName(git_info.repo_name, platform)
        ditto_deb.buildInsideDocker(image_name, build_config.docker_file)
        ditto_deb.generatePackageInsideDocker(image_name, version, revision)
        ditto_deb.publishPackageToS3(build_config.apt_test_repo,
                                     build_config.dist,
                                     build_config.nexus_test)
      }

      stage("Installing from ${platform} repo and test") {
        ditto_deb.installPackageInsideDocker(
          image_name, build_config.apt_test_repo, build_config.dist,
          version, revision)
      }
    }
  }
}

stage("Tag and deploy?") {
  deploy_mode = "SKIP"
  if (git_info.is_release_branch) {
    deploy_mode = input(
      message: "User input required",
      parameters: [
        choice(
          name: "Deploy \"${version}\" at hash \"${git_info.commit}\"?",
          choices: ditto_deb.getDeployChoices(origin).join("\n"))])
  }
}

node('static-minion') {
  stage("Building and publishing to rc or release") {
    if (!(deploy_mode == "RC" || deploy_mode == "RELEASE")) return;

    BUILD_CONFIGS.each { platform, build_config ->
      dir(platform) {
        if (deploy_mode == "RC") {
          tag = ditto_git.getRcTag(version)
          revision = ditto_deb.buildRcRevisionString(version)
        } else if (deploy_mode == "RELEASE") {
          tag = ditto_git.getReleaseTag(origin, version)
          revision = ditto_deb.buildReleaseRevisionString(origin, version)
        }

        image_name =
          ditto_utils.buildDockerImageName(git_info.repo_name, platform)
        if (deploy_mode == "RC") {
          apt_repo_to_publish = build_config.apt_test_repo
          nexus_repo_to_publish = build_config.nexus_test
        } else {
          apt_repo_to_publish = build_config.apt_prod_repo
          nexus_repo_to_publish = build_config.nexus_prod
        }

        ditto_deb.generatePackageInsideDocker(image_name, version, revision)
        ditto_deb.publishPackageToS3(
          apt_repo_to_publish,
          build_config.dist,
          nexus_repo_to_publish)
      }
    }

    // Do this once at the end of Jenkinsfile, since it changes state
    // of rc and build numbers.
    dir(FIRST_PLATFORM) {
      ditto_git.pushTag(tag, GIT_CREDENTIALS_ID)
    }
  }

  stage("Cleaning up") {
    deleteDir()
  }
}
