"""
conanfile.py — Conan 2 recipe for the MinIO C++ PoC.

Manages the aws-sdk-cpp dependency (S3 component only) and generates
the CMakeDeps / CMakeToolchain files consumed by Meson via cmake_find_package.

Usage:
    conan install . --output-folder=build --build=missing -s build_type=Release
"""

from conan import ConanFile
from conan.tools.cmake import cmake_layout


class MinIOCppPoC(ConanFile):
    name        = "minio-cpp-poc"
    version     = "0.1.0"
    description = "Proof-of-concept: MinIO / AIStor access from C++ via aws-sdk-cpp"
    license     = "MIT"
    settings    = "os", "compiler", "build_type", "arch"

    # Only the S3 component — keeps compile times short
    requires = "aws-sdk-cpp/1.11.352"

    options = {
        "shared": [True, False],
        "fPIC":   [True, False],
    }
    default_options = {
        "shared":                       False,
        "fPIC":                         True,
        # Build only the S3 service (and its transitive deps)
        "aws-sdk-cpp/*:s3":             True,
        "aws-sdk-cpp/*:shared":         False,
        # Disable every other service to avoid long build times
        "aws-sdk-cpp/*:access-management":         False,
        "aws-sdk-cpp/*:acm":                       False,
        "aws-sdk-cpp/*:apigateway":                False,
        "aws-sdk-cpp/*:application-autoscaling":   False,
        "aws-sdk-cpp/*:autoscaling":               False,
        "aws-sdk-cpp/*:cloudformation":            False,
        "aws-sdk-cpp/*:cloudfront":                False,
        "aws-sdk-cpp/*:cloudtrail":                False,
        "aws-sdk-cpp/*:cloudwatch":                False,
        "aws-sdk-cpp/*:codedeploy":                False,
        "aws-sdk-cpp/*:cognito-identity":          False,
        "aws-sdk-cpp/*:config":                    False,
        "aws-sdk-cpp/*:dynamodb":                  False,
        "aws-sdk-cpp/*:ec2":                       False,
        "aws-sdk-cpp/*:ecr":                       False,
        "aws-sdk-cpp/*:ecs":                       False,
        "aws-sdk-cpp/*:elasticache":               False,
        "aws-sdk-cpp/*:elasticloadbalancing":      False,
        "aws-sdk-cpp/*:glacier":                   False,
        "aws-sdk-cpp/*:glue":                      False,
        "aws-sdk-cpp/*:iam":                       False,
        "aws-sdk-cpp/*:kinesis":                   False,
        "aws-sdk-cpp/*:kms":                       False,
        "aws-sdk-cpp/*:lambda":                    False,
        "aws-sdk-cpp/*:logs":                      False,
        "aws-sdk-cpp/*:monitoring":                False,
        "aws-sdk-cpp/*:rds":                       False,
        "aws-sdk-cpp/*:redshift":                  False,
        "aws-sdk-cpp/*:route53":                   False,
        "aws-sdk-cpp/*:s3control":                 False,
        "aws-sdk-cpp/*:secretsmanager":            False,
        "aws-sdk-cpp/*:ses":                       False,
        "aws-sdk-cpp/*:sns":                       False,
        "aws-sdk-cpp/*:sqs":                       False,
        "aws-sdk-cpp/*:ssm":                       False,
        "aws-sdk-cpp/*:sts":                       False,
        "aws-sdk-cpp/*:transfer":                  False,
        "aws-sdk-cpp/*:waf":                       False,
        "aws-sdk-cpp/*:xray":                      False,
    }

    generators = [
        "CMakeDeps",        # generates Find<Pkg>.cmake — consumed by Meson's cmake module
        "CMakeToolchain",   # sets compiler / sysroot flags
    ]

    def layout(self):
        cmake_layout(self, build_folder="build")