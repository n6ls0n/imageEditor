pipeline {
    agent any

    parameters {
        booleanParam(name: 'PUSH_TO_REGISTRY', defaultValue: true, description: 'Push images to Docker registry?')
    }

    environment {
        DEPLOY_DIR = credentials('IMAGEEDITOR_DEPLOY_DIR')
        DEPLOY_SERVER = credentials('MAIN_SERVER')
        DOCKER_IMAGE = 'nelsoncnwauche/images:imageeditor-latest'
        GIT_CREDS = 'dcf54507-1c53-47e5-9869-a1015d823d32'
        DOCKER_CREDS = '4744871b-b8cc-46c9-9112-26272b68eeba'
        SSH_CREDS = 'ssh-credentials'
    }

    stages {
        stage('Checkout') {
            steps {
                checkout([
                    $class: 'GitSCM',
                    branches: [[name: '*/master']],
                    userRemoteConfigs: [[
                        url: 'https://github.com/n6ls0n/imageEditor.git',
                        credentialsId: env.GIT_CREDS
                    ]],
                ])
            }
        }

        stage('Build Docker Images') {
            steps {
                script {
                    sh "docker build -f Dockerfile -t ${env.DOCKER_IMAGE} ."
                }
            }
        }

        stage('Push Docker Images') {
            when {
                expression { params.PUSH_TO_REGISTRY }
            }
            steps {
                script {
                    try {
                        docker.withRegistry('', env.DOCKER_CREDS) {
                            sh "docker push ${env.DOCKER_IMAGE}"
                        }
                    } catch (Exception e) {
                        error "Failed to push images to registry: ${e.getMessage()}"
                    }
                }
            }
        }

        stage('Add Host Key') {
            steps {
                script {
                    sh """
                        mkdir -p ~/.ssh
                        ssh-keyscan -H ${env.DEPLOY_SERVER.split('@')[1]} >> ~/.ssh/known_hosts
                    """
                }
            }
        }

        stage('Prepare Deployment Directory') {
            steps {
                script {
                    try {
                        sshagent(credentials: [env.SSH_CREDS]) {
                            sh """
                                ssh ${env.DEPLOY_SERVER} '
                                    mkdir -p ${env.DEPLOY_DIR}
                                    chmod -R 775 ${env.DEPLOY_DIR}
                                    chmod g+s ${env.DEPLOY_DIR}
                                    chown nelson:nelson ${env.DEPLOY_DIR}
                                '
                            """
                        }
                    } catch (Exception e) {
                        error "Failed to prepare deployment directory: ${e.getMessage()}"
                    }
                }
            }
        }

        stage('Deploy to Server') {
            when {
                expression {
                    return currentBuild.result == null || currentBuild.result == 'SUCCESS'
                }
            }
            steps {
                script {
                    try {
                        docker.withRegistry('', env.DOCKER_CREDS) {
                            sshagent(credentials: [env.SSH_CREDS]) {
                                sh """
                                    ssh ${env.DEPLOY_SERVER} '
                                        cd ${env.DEPLOY_DIR}
                                        docker stop imageeditor || true
                                        docker rm imageeditor || true
                                        docker pull ${env.DOCKER_IMAGE}
                                        docker run -d \
                                            --name imageeditor \
                                            --restart unless-stopped \
                                            -p 7060:7060 \
                                            ${env.DOCKER_IMAGE}
                                    '
                                """
                            }
                        }
                        echo "Deployment successful"
                    } catch (Exception e) {
                        error "Deployment failed: ${e.getMessage()}"
                    }
                }
            }
        }
    }

    post {
        always {
            script {
                sshagent(credentials: [env.SSH_CREDS]) {
                    sh """
                        ssh ${env.DEPLOY_SERVER} '
                            rm -rf ${env.DEPLOY_DIR}@tmp || true
                        '
                    """
                }
            }
        }
        failure {
            echo "Pipeline failed! Check the logs for details."
        }
    }
}